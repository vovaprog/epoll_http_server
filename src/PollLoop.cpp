#include <PollLoop.h>
#include <TimeUtils.h>

#include <sys/epoll.h>
#include <signal.h>
#include <sys/eventfd.h>


int PollLoop::init(ServerBase *srv, ServerParameters *params)
{
    this->srv = srv;
    this->parameters = params;
    log = srv->log;

    numOfPollFds.store(0);

    strcpy(fileNameBuffer, params->rootFolder.c_str());
    strcat(fileNameBuffer, "/");
    rootFolderLength = strlen(fileNameBuffer);


    if(initDataStructs() != 0)
    {
        destroy();
        return -1;
    }


    epollFd = epoll_create1(0);
    if(epollFd == -1)
    {
        log->error("epoll_create1 failed: %s\n", strerror(errno));
        destroy();
        return -1;
    }


    newFdExecutor.init(this);
    if(createEventFd() != 0)
    {
        destroy();
        return -1;
    }

    serverExecutor.init(this);
    sslServerExecutor.init(this);
    requestExecutor.init(this);
    fileExecutor.init(this);
    uwsgiExecutor.init(this);
    sslRequestExecutor.init(this);
    sslFileExecutor.init(this);
    sslUwsgiExecutor.init(this);


    return 0;
}


int PollLoop::run()
{
    signal(SIGPIPE, SIG_IGN);

    runFlag.store(true);

    while(epollFd > 0 && runFlag.load())
    {
        int nEvents = epoll_wait(epollFd, events, MAX_EVENTS, parameters->executorTimeoutMillis);
        if(nEvents == -1)
        {
            if(errno == EINTR)
            {
                continue;
            }
            else
            {
                log->error("epoll_wait failed: %s\n", strerror(errno));
                destroy();
                return -1;
            }
        }
        if(!runFlag.load())
        {
            break;
        }

        long long int curMillis = getMilliseconds();

        for(int i = 0; i < nEvents; ++i)
        {
            PollData *pollData = static_cast<PollData*>(events[i].data.ptr);
            ExecutorData &execData = execDatas[pollData->executorDataIndex];

            if(execData.state != ExecutorData::State::invalid)
            {
                if((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLERR))
                {
                    removeExecutorData(&execData);
                }
                else
                {
                    ProcessResult result = execData.pExecutor->process(execData, pollData->fd, events[i].events);

                    if(result == ProcessResult::removeExecutor)
                    {
                        removeExecutorData(&execData);
                    }
                    else if(result == ProcessResult::shutdown)
                    {
                        destroy();
                        return -1;
                    }
                    else
                    {
                        if(execData.badOperationCounter > ExecutorData::MAX_BAD_OPERATION_COUNTER)
                        {
                            log->warning("big badOperationCounter. destroy executor\n");
                            removeExecutorData(&execData);
                        }
                        else
                        {
                            execData.lastProcessTime = curMillis;
                        }
                    }
                }
            }
        }

        if(curMillis - lastCheckTimeoutMillis >= parameters->executorTimeoutMillis)
        {
            checkTimeout(curMillis);
            if(parameters->logStats)
            {
                logStats();
            }
        }
    }

    destroy();
    return 0;
}


void PollLoop::stop()
{
    runFlag.store(false);
    eventfd_write(eventFd, 1);
}


int PollLoop::enqueueClientFd(int fd, ExecutorType execType)
{
    {
        std::lock_guard<std::mutex> lock(newFdsMutex);

        NewFdData fdData;
        fdData.fd = fd;
        fdData.execType = execType;

        if(!newFdsQueue.push(fdData))
        {
            return -1;
        }
    }

    eventfd_write(eventFd, 1);

    return 0;
}


int PollLoop::checkNewFd()
{
    NewFdData fdData;
    while(newFdsQueue.pop(fdData))
    {
        if(createRequestExecutorInternal(fdData.fd, fdData.execType) != 0)
        {
            close(fdData.fd);
        }
    }
    return 0;
}


int PollLoop::createRequestExecutor(int fd, ExecutorType execType)
{
    if(parameters->threadCount == 1)
    {
        return createRequestExecutorInternal(fd, execType);
    }
    else
    {
        return srv->createRequestExecutor(fd, execType);
    }
}


int PollLoop::numberOfPollFds()
{
    return numOfPollFds.load();
}


int PollLoop::addPollFd(ExecutorData &data, int fd, int events)
{
    if(emptyPollDatas.size() == 0)
    {
        log->error("too many connections\n");
        return -1;
    }

    if(fd != data.fd0 && fd != data.fd1)
    {
        log->error("invalid argument\n");
        return -1;
    }

    int pollIndex = emptyPollDatas.top();

    epoll_event ev;
    ev.events = (events | EPOLLRDHUP | EPOLLERR);
    ev.data.ptr = &pollDatas[pollIndex];
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        log->error("epoll_ctl add failed: %s\n", strerror(errno));
        return -1;
    }

    pollDatas[pollIndex].fd = fd;
    pollDatas[pollIndex].executorDataIndex = data.index;

    if(fd == data.fd0)
    {
        data.pollIndexFd0 = pollIndex;
    }
    else if(fd == data.fd1)
    {
        data.pollIndexFd1 = pollIndex;
    }

    emptyPollDatas.pop();
    ++numOfPollFds;

    return 0;
}


int PollLoop::editPollFd(ExecutorData &data, int fd, int events)
{
    int pollIndex = -1;
    if(fd == data.fd0)
    {
        pollIndex = data.pollIndexFd0;
    }
    else if(fd == data.fd1)
    {
        pollIndex = data.pollIndexFd1;
    }
    else
    {
        return -1;
    }

    epoll_event ev;
    ev.events = (events | EPOLLRDHUP | EPOLLERR);
    ev.data.ptr = &pollDatas[pollIndex];
    if(epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        log->error("epoll_ctl mod failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


int PollLoop::removePollFd(ExecutorData &data, int fd)
{
    if(epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) != 0)
    {
        log->error("epoll_ctl del failed: %s\n", strerror(errno));
        return -1;
    }
    if(fd == data.fd0)
    {
        emptyPollDatas.push(data.pollIndexFd0);
        --numOfPollFds;
    }
    else if(fd == data.fd1)
    {
        emptyPollDatas.push(data.pollIndexFd1);
        --numOfPollFds;
    }
    return 0;
}


int PollLoop::listenPort(int port, ExecutorType execType)
{
    ExecutorData *pExecData = createExecutorData();

    pExecData->pExecutor = getExecutor(execType);
    pExecData->port = port;


    if(pExecData->pExecutor->up(*pExecData) != 0)
    {
        removeExecutorData(pExecData);
        return -1;
    }

    return 0;
}


void PollLoop::checkTimeout(long long int curMillis)
{
    removeExecDatas.erase(removeExecDatas.begin(), removeExecDatas.end());
    removeExecDatas.reserve(usedExecDatas.size());

    for(int i : usedExecDatas)
    {
        if(execDatas[i].removeOnTimeout)
        {
            if(curMillis - execDatas[i].lastProcessTime > parameters->executorTimeoutMillis)
            {
                //can't remove here, because removeExecutorData modifies usedExecDatas
                removeExecDatas.push_back(i);
            }
        }
    }

    for(int i : removeExecDatas)
    {
        removeExecutorData(&execDatas[i]);
    }

    lastCheckTimeoutMillis = curMillis;
}


int PollLoop::createEventFd()
{
    eventFd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE); //check flags !!!

    if(eventFd < 0)
    {
        log->error("eventfd failed: %s\n", strerror(errno));
        destroy();
        return -1;
    }

    ExecutorData *execData = createExecutorData();
    execData->fd0 = eventFd;
    execData->pExecutor = &newFdExecutor;

    if(addPollFd(*execData, execData->fd0, EPOLLIN) != 0)
    {
        removeExecutorData(execData);
        return -1;
    }

    execData->pExecutor->up(*execData);

    return 0;
}


void PollLoop::destroy()
{
    if(execDatas != nullptr)
    {
        delete[] execDatas;
        execDatas = nullptr;
    }
    if(pollDatas != nullptr)
    {
        delete[] pollDatas;
        pollDatas = nullptr;
    }
    if(events != nullptr)
    {
        delete[] events;
        events = nullptr;
    }
    if(epollFd > 0)
    {
        close(epollFd);
        epollFd = -1;
    }
    if(eventFd > 0)
    {
        close(eventFd);
        eventFd = -1;
    }
}


int PollLoop::initDataStructs()
{
    MAX_EXECUTORS = parameters->maxClients;
    MAX_EVENTS = MAX_EXECUTORS * 2;

    execDatas = new ExecutorData[MAX_EXECUTORS];
    pollDatas = new PollData[MAX_EVENTS];
    events = new epoll_event[MAX_EVENTS];

    for(int i = MAX_EXECUTORS - 1; i >= 0; --i)
    {
        emptyExecDatas.push(i);
    }
    for(int i = MAX_EVENTS - 1; i >= 0; --i)
    {
        emptyPollDatas.push(i);
    }
    for(int i = 0; i < MAX_EXECUTORS; ++i)
    {
        execDatas[i].index = i;
    }
    usedExecDatas.clear();
    return 0;
}


Executor* PollLoop::getExecutor(ExecutorType execType)
{
    switch(execType)
    {
    case ExecutorType::request:
        return &requestExecutor;
    case ExecutorType::file:
        return &fileExecutor;
    case ExecutorType::sslFile:
        return &sslFileExecutor;
    case ExecutorType::uwsgi:
        return &uwsgiExecutor;
    case ExecutorType::server:
        return &serverExecutor;
    case ExecutorType::serverSsl:
        return &sslServerExecutor;
    case ExecutorType::requestSsl:
        return &sslRequestExecutor;
    case ExecutorType::sslUwsgi:
        return &sslUwsgiExecutor;
    default:
        return nullptr;
    }
}


ExecutorData* PollLoop::createExecutorData()
{
    if(emptyExecDatas.size() == 0)
    {
        log->error("too many connections\n");
        return nullptr;
    }

    int execIndex = emptyExecDatas.top();
    emptyExecDatas.pop();
    usedExecDatas.insert(execIndex);

    return &execDatas[execIndex];
}


void PollLoop::removeExecutorData(ExecutorData *execData)
{
    if(execData->pollIndexFd0 >= 0)
    {
        emptyPollDatas.push(execData->pollIndexFd0);
        --numOfPollFds;
    }
    if(execData->pollIndexFd1 >= 0)
    {
        emptyPollDatas.push(execData->pollIndexFd1);
        --numOfPollFds;
    }

    execData->down();

    emptyExecDatas.push(execData->index);
    usedExecDatas.erase(execData->index);
}


int PollLoop::createRequestExecutorInternal(int fd, ExecutorType execType)
{
    ExecutorData *pExecData = createExecutorData();

    if(pExecData == nullptr)
    {
        return -1;
    }

    pExecData->pExecutor = getExecutor(execType);
    pExecData->fd0 = fd;

    if(pExecData->pExecutor->up(*pExecData) != 0)
    {
        removeExecutorData(pExecData);
        return -1;
    }

    return 0;
}


void PollLoop::logStats()
{
    unsigned long long int tid = pthread_self();
    char tidBuf[30];
    snprintf(tidBuf, 30, "%llu", tid);

    log->info("[%s]<<<<<<<\n",tidBuf);

    for(int execIndex : usedExecDatas)
    {
        execDatas[execIndex].writeLog(log, Log::Level::info, tidBuf);
    }

    log->info("[%s]>>>>>>>\n",tidBuf);
}

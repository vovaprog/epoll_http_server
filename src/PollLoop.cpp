#include <PollLoop.h>
#include <TimeUtils.h>

#include <sys/epoll.h>
#include <sys/eventfd.h>


int PollLoop::init(ServerBase *srv, ServerParameters *params)
{
    this->srv = srv;
    this->parameters = params;
    log = srv->log;

    numOfPollFds.store(0);


    snprintf(fileNameBuffer, MAX_FILE_NAME, "%s/", params->rootFolder.c_str());
    rootFolderLength = strlen(fileNameBuffer);


    if(initDataStructs(params) != 0)
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
    proxyExecutor.init(this);
    sslRequestExecutor.init(this);
    sslFileExecutor.init(this);
    sslProxyExecutor.init(this);


    return 0;
}


int PollLoop::run()
{
    runFlag.store(true);

    while(epollFd > 0 && runFlag.load())
    {
        int nEvents = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, parameters->executorTimeoutMillis);
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
            ExecutorData *execData = pollData->execData;

            if(execData == nullptr)
            {
                log->error("execData is null!\n");
                continue;
            }

            if(execData->state != ExecutorData::State::invalid)
            {
                if(((events[i].events & EPOLLRDHUP) || (events[i].events & EPOLLERR)) && pollData->fd == execData->fd0)
                {
                    log->debug("received EPOLLRDHUP or EPOLLERR event on fd\n");
                    removeExecutorData(execData);
                }
                else
                {
                    ProcessResult result = execData->pExecutor->process(*execData, pollData->fd, events[i].events);

                    if(result == ProcessResult::removeExecutorOk)
                    {
                        removeExecutorData(execData);
                    }
                    else if(result == ProcessResult::removeExecutorError)
                    {
                        execData->writeLog(log, Log::Level::warning, "removeExecutorError");
                        removeExecutorData(execData);
                    }
                    else if(result == ProcessResult::shutdown)
                    {
                        destroy();
                        return -1;
                    }
                    else
                    {
                        if(execData->retryCounter > ExecutorData::MAX_RETRY_COUNTER)
                        {
                            log->warning("retryCounter > MAX_RETRY_COUNTER. destroy executor\n");
                            removeExecutorData(execData);
                        }
                        else
                        {
                            execData->lastProcessTime = curMillis;
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


int PollLoop::addPollFd(ExecutorData &data, int fd, int events)
{
    if(fd != data.fd0 && fd != data.fd1)
    {
        log->error("addPollFd invalid fd argument\n");
        return -1;
    }

    if(fd == data.fd0 && data.pollData0 != nullptr )
    {
        log->error("addPollFd: pollData0 != nullptr\n");
        return -1;
    }

    if(fd == data.fd1 && data.pollData1 != nullptr)
    {
        log->error("addPollFd: pollData1 != nullptr\n");
        return -1;
    }


    PollData *pollData = pollDatas.allocate();
    if(pollData == nullptr)
    {
        log->error("pollDatas.allocate failed\n");
        return -1;
    }
    ++numOfPollFds;


    epoll_event ev;
    ev.events = (events | EPOLLRDHUP | EPOLLERR);
    ev.data.ptr = pollData;
    if(epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        log->error("epoll_ctl add failed: %s\n", strerror(errno));
        pollDatas.free(pollData);
        --numOfPollFds;
        return -1;
    }

    pollData->fd = fd;
    pollData->execData = &data;

    if(fd == data.fd0)
    {
        data.pollData0 = pollData;
    }
    else if(fd == data.fd1)
    {
        data.pollData1 = pollData;
    }

    return 0;
}


int PollLoop::editPollFd(ExecutorData &data, int fd, int events)
{
    PollData *pollData = nullptr;
    if(fd == data.fd0)
    {
        pollData = data.pollData0;
    }
    else if(fd == data.fd1)
    {
        pollData = data.pollData1;
    }

    if(pollData == nullptr)
    {
        log->error("editPollFd: pollData is null\n");
        return -1;
    }

    epoll_event ev;
    ev.events = (events | EPOLLRDHUP | EPOLLERR);
    ev.data.ptr = pollData;
    if(epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        log->error("epoll_ctl mod failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


int PollLoop::removePollFd(ExecutorData &data, int fd)
{
    PollData *pollData = nullptr;

    if(fd == data.fd0)
    {
        pollData = data.pollData0;
    }
    else if(fd == data.fd1)
    {
        pollData = data.pollData1;
    }

    if(pollData == nullptr)
    {
        data.writeLog(log, Log::Level::error, "removePollFd called with invalid fd");
        return -1;
    }

    if(epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL) != 0)
    {
        log->error("epoll_ctl del failed: %s\n", strerror(errno));
        return -1;
    }

    pollDatas.free(pollData);
    --numOfPollFds;

    if(pollData == data.pollData0)
    {
        data.pollData0 = nullptr;
    }
    else if(pollData == data.pollData1)
    {
        data.pollData1 = nullptr;
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

    BlockStorage<ExecutorData>::Iterator iter = execDatas.begin();
    BlockStorage<ExecutorData>::Iterator end = execDatas.end();

    for(; iter != end ; ++iter)
    {
        ExecutorData *execData = iter.pointer();

        if(execData == nullptr)
        {
            log->error("checkTimeout - executor data is null\n");
        }
        else
        {
            if(execData->removeOnTimeout)
            {
                if((curMillis - execData->lastProcessTime > parameters->executorTimeoutMillis) ||
                        (curMillis - execData->createTime > ExecutorData::MAX_TIME_TO_LIVE_MILLIS))
                {
                    removeExecDatas.push_back(execData);
                    execData->writeLog(log, Log::Level::debug, "timeout remove executor");
                }
            }
        }
    }

    for(ExecutorData * execData : removeExecDatas)
    {
        removeExecutorData(execData);
    }

    lastCheckTimeoutMillis = curMillis;
}


int PollLoop::createEventFd()
{
    eventFd = eventfd(0, EFD_NONBLOCK | EFD_SEMAPHORE);

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
    execDatas.destroy();
    pollDatas.destroy();

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


int PollLoop::initDataStructs(const ServerParameters *params)
{
    if(execDatas.init(params->maxAllocationBlocks, params->allocationBlockSize) != 0)
    {
        return -1;
    }
    if(pollDatas.init(params->maxAllocationBlocks, params->allocationBlockSize * 2) != 0)
    {
        return -1;
    }

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
    case ExecutorType::proxy:
        return &proxyExecutor;
    case ExecutorType::server:
        return &serverExecutor;
    case ExecutorType::serverSsl:
        return &sslServerExecutor;
    case ExecutorType::requestSsl:
        return &sslRequestExecutor;
    case ExecutorType::sslProxy:
        return &sslProxyExecutor;
    default:
        return nullptr;
    }
}


ExecutorData* PollLoop::createExecutorData()
{
    ExecutorData *execData = execDatas.allocate();

    if(execData == nullptr)
    {
        log->error("execDatas.allocate failed\n");
        return nullptr;
    }

    long long int curMillis = getMilliseconds();

    execData->createTime = curMillis;
    execData->lastProcessTime = curMillis;

    return execData;
}


void PollLoop::removeExecutorData(ExecutorData *execData)
{
    execData->writeLog(log, Log::Level::debug, "remove executor");

    if(execData->pollData0 != nullptr)
    {
        removePollFd(*execData, execData->fd0);
    }
    if(execData->pollData1 != nullptr)
    {
        removePollFd(*execData, execData->fd1);
    }

    execData->down();

    execDatas.free(execData);
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


int PollLoop::closeFd(ExecutorData &data, int fd)
{
    if(fd == data.fd0)
    {
        if(data.pollData0 != nullptr)
        {
            removePollFd(data, data.fd0);
        }
        close(data.fd0);
        data.fd0 = -1;
        return 0;
    }
    else if(fd == data.fd1)
    {
        if(data.pollData1 != nullptr)
        {
            removePollFd(data, data.fd1);
        }
        close(data.fd1);
        data.fd1 = -1;
        return 0;
    }

    log->error("closeFd - invalid arguments\n");
    return -1;
}


int PollLoop::numberOfPollFds() const
{
    return numOfPollFds.load();
}


void PollLoop::logStats()
{
    unsigned long long int tid = pthread_self();
    char tidBuf[30];
    snprintf(tidBuf, 30, "[%llu]", tid);

    log->info("%s<<<<<<<\n", tidBuf);

    BlockStorage<ExecutorData>::StorageInfo siExec;
    if(execDatas.getStorageInfo(siExec) == 0)
    {
        log->info("%s   executors. blocks: %d   empty: %d   blocksize: %d   maxBlocks: %d\n",
                  tidBuf, siExec.allocatedBlocks, siExec.emptyItems, siExec.blockSize, siExec.maxBlocks);
    }

    BlockStorage<PollData>::StorageInfo siPoll;
    if(pollDatas.getStorageInfo(siPoll) == 0)
    {
        log->info("%s   poll data. blocks: %d   empty: %d   blocksize: %d   maxBlocks: %d\n",
                  tidBuf, siPoll.allocatedBlocks, siPoll.emptyItems, siPoll.blockSize, siPoll.maxBlocks);
    }


    BlockStorage<ExecutorData>::Iterator iter = execDatas.begin();
    BlockStorage<ExecutorData>::Iterator end = execDatas.end();

    for(; iter != end ; ++iter)
    {
        ExecutorData *execData = iter.pointer();

        if(execData == nullptr)
        {
            log->error("logStats - exec data is null\n");
        }
        else
        {
            execData->writeLog(log, Log::Level::info, tidBuf);
        }      
    }

    log->info("%s>>>>>>>\n", tidBuf);
}


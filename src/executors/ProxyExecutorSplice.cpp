#include <ProxyExecutorSplice.h>
#include <PollLoopBase.h>
#include <NetworkUtils.h>

#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>

int ProxyExecutorSplice::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int ProxyExecutorSplice::up(ExecutorData &data)
{
    data.removeOnTimeout = true;

    bool connected = false;

    data.fd1 = socketConnectNonBlock(data.proxy->address.c_str(), data.proxy->port, connected, log);

    if(data.fd1 < 0)
    {
        return -1;
    }

    if(loop->addPollFd(data, data.fd1, EPOLLOUT) != 0)
    {
        return -1;
    }

    if(loop->removePollFd(data, data.fd0) != 0)
    {
        return -1;
    }

    if(connected)
    {
        data.state = ExecutorData::State::forwardRequest;
    }
    else
    {
        data.state = ExecutorData::State::waitConnect;
    }

    return 0;
}


ProcessResult ProxyExecutorSplice::process(ExecutorData &data, int fd, int events)
{
    if(data.state == ExecutorData::State::waitConnect && fd == data.fd1 && (events & EPOLLOUT))
    {
        return process_waitConnect(data);
    }
    if(data.state == ExecutorData::State::forwardRequest && fd == data.fd1 && (events & EPOLLOUT))
    {
        return process_forwardRequest(data);
    }
    if(data.state == ExecutorData::State::forwardResponse && fd == data.fd1 && (events & EPOLLIN))
    {
        return process_forwardResponseRead(data);
    }
    if(data.state == ExecutorData::State::forwardResponse && fd == data.fd0 && (events & EPOLLOUT))
    {
        return process_forwardResponseWrite(data);
    }
    if(data.state == ExecutorData::State::forwardResponseOnlyWrite && fd == data.fd0 && (events & EPOLLOUT))
    {
        return process_forwardResponseOnlyWrite(data);
    }

    loop->log->warning("invalid process call (proxy)\n");
    return ProcessResult::removeExecutorError;
}


ProcessResult ProxyExecutorSplice::process_waitConnect(ExecutorData &data)
{
    int socketError = socketConnectNonBlockCheck(data.fd1, log);

    if(socketError == 0)
    {
        data.state = ExecutorData::State::forwardRequest;
        return process_forwardRequest(data);
    }
    else if(socketError == EINPROGRESS)
    {
        ++data.retryCounter;
        return ProcessResult::ok;
    }
    else
    {
        log->error("ProxyExecutor::process_waitConnect socketConnectNonBlockCheck failed: %d\n", socketError);
        return ProcessResult::removeExecutorError;
    }
}


ProcessResult ProxyExecutorSplice::process_forwardRequest(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        int bytesWritten = write(data.fd1, p, size);

        if(bytesWritten <= 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                ++data.retryCounter;
                return ProcessResult::ok;
            }
            else
            {
                log->error("write failed: %s\n", strerror(errno));
                return ProcessResult::removeExecutorError;
            }
        }
        data.retryCounter = 0;

        data.buffer.endRead(bytesWritten);

        if(bytesWritten == size)
        {
            data.buffer.clear();

            data.state = ExecutorData::State::forwardResponse;

            if(loop->addPollFd(data, data.fd0, EPOLLOUT) != 0)
            {
                return ProcessResult::removeExecutorError;
            }
            if(loop->editPollFd(data, data.fd1, EPOLLIN) != 0)
            {
                return ProcessResult::removeExecutorError;
            }
            if(openPipe(data) != 0)
            {
                return ProcessResult::removeExecutorError;
            }
            return ProcessResult::ok;
        }

        return ProcessResult::ok;
    }
    else
    {
        log->error("buffer.startRead failed\n");
        return ProcessResult::removeExecutorError;
    }
}


ProcessResult ProxyExecutorSplice::process_forwardResponseRead(ExecutorData &data)
{
    int bytes = splice(data.fd1 , NULL, data.pipeWriteFd, NULL, 10000, SPLICE_F_NONBLOCK | SPLICE_F_MORE);

    if(bytes == 0)
    {
        data.state = ExecutorData::State::forwardResponseOnlyWrite;
        return ProcessResult::ok;
    }
    else if(bytes < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            ++data.retryCounter;
            return ProcessResult::ok;
        }
        else
        {
            log->error("splice failed: %s\n", strerror(errno));
            return ProcessResult::removeExecutorError;
        }
    }
    else
    {
        data.retryCounter = 0;
    }

    return ProcessResult::ok;
}

ProcessResult ProxyExecutorSplice::process_forwardResponseWrite(ExecutorData &data)
{
    int bytes = splice(data.pipeReadFd, NULL, data.fd0, NULL, 10000, SPLICE_F_NONBLOCK | SPLICE_F_MORE);

    if(bytes == 0)
    {
        return ProcessResult::ok;
    }
    else if(bytes < 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            ++data.retryCounter;
            return ProcessResult::ok;
        }
        else
        {
            log->error("splice failed: %s\n", strerror(errno));
            return ProcessResult::removeExecutorError;
        }
    }
    else
    {
        data.retryCounter = 0;
    }

    return ProcessResult::ok;
}


ProcessResult ProxyExecutorSplice::process_forwardResponseOnlyWrite(ExecutorData &data)
{
    int bytes = splice(data.pipeReadFd, NULL, data.fd0, NULL, 10000, SPLICE_F_NONBLOCK | SPLICE_F_MORE);

    if(bytes == 0)
    {
        return ProcessResult::removeExecutorOk;
    }
    else if(bytes < 0)
    {
        log->error("splice failed: %s\n", strerror(errno));
        return ProcessResult::removeExecutorError;
    }

    return ProcessResult::ok;
}


int ProxyExecutorSplice::openPipe(ExecutorData &data)
{
    int pipeFd[2];

    if(pipe2(pipeFd, O_NONBLOCK) != 0)
    {
        log->error("pipe2 failed: %s\n", strerror(errno));
        return -1;
    }

    data.pipeReadFd = pipeFd[0];
    data.pipeWriteFd = pipeFd[1];

    return 0;
}

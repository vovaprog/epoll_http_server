#include <ProxyExecutor.h>
#include <PollLoopBase.h>
#include <NetworkUtils.h>

#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>

int ProxyExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int ProxyExecutor::up(ExecutorData &data)
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


ProcessResult ProxyExecutor::process(ExecutorData &data, int fd, int events)
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


ProcessResult ProxyExecutor::process_forwardRequest(ExecutorData &data)
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
            return ProcessResult::ok;
        }

        return ProcessResult::ok;
    }
    else
    {
        loop->log->error("buffer.startRead failed\n");
        return ProcessResult::removeExecutorError;
    }
}


ProcessResult ProxyExecutor::process_forwardResponseRead(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startWrite(p, size))
    {
        int bytesRead = read(data.fd1, p, size);

        if(bytesRead <= 0)
        {
            if(bytesRead == 0)
            {
                close(data.fd1);
                data.fd1 = -1;
                data.state = ExecutorData::State::forwardResponseOnlyWrite;
                return ProcessResult::ok;
            }
            else if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                ++data.retryCounter;
                return ProcessResult::ok;
            }
            else
            {
                log->error("ProxyExecutor::process_forwardResponseRead   read failed: %s\n", strerror(errno));
                return ProcessResult::removeExecutorError;
            }
        }
        else
        {
            data.retryCounter = 0;
            data.buffer.endWrite(bytesRead);
            return ProcessResult::ok;
        }
    }
    else
    {
        return ProcessResult::ok;
    }
}


ProcessResult ProxyExecutor::process_forwardResponseWrite(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        int errorCode = 0;
        int bytesWritten = writeFd0(data, p, size, errorCode);

        if(bytesWritten <= 0)
        {
            if(errorCode == EAGAIN || errorCode == EWOULDBLOCK)
            {
                ++data.retryCounter;
                return ProcessResult::ok;
            }
            else
            {
                log->error("writeFd0 failed: %s\n", strerror(errorCode));
                return ProcessResult::removeExecutorError;
            }
        }
        data.retryCounter = 0;

        data.buffer.endRead(bytesWritten);
    }

    return ProcessResult::ok;
}


ProcessResult ProxyExecutor::process_forwardResponseOnlyWrite(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        int errorCode = 0;
        int bytesWritten = writeFd0(data, p, size, errorCode);

        if(bytesWritten <= 0)
        {
            if(errorCode == EAGAIN || errorCode == EWOULDBLOCK)
            {
                ++data.retryCounter;
                return ProcessResult::ok;
            }
            else
            {
                log->error("writeFd0 failed: %s\n", strerror(errorCode));
                return ProcessResult::removeExecutorError;
            }
        }
        data.retryCounter = 0;

        data.buffer.endRead(bytesWritten);

        if(bytesWritten == size && !data.buffer.readAvailable())
        {
            return ProcessResult::removeExecutorOk;
        }
        else
        {
            return ProcessResult::ok;
        }
    }
    else
    {
        return ProcessResult::removeExecutorOk;
    }
}

ProcessResult ProxyExecutor::process_waitConnect(ExecutorData &data)
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

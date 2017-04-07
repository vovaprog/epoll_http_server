#include <ProxyExecutorReadWrite.h>
#include <PollLoopBase.h>
#include <NetworkUtils.h>

#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>

int ProxyExecutorReadWrite::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int ProxyExecutorReadWrite::up(ExecutorData &data)
{
    data.removeOnTimeout = true;

    bool connected = false;

    if(data.proxy == nullptr)
    {
        log->error("proxy parameters are not set\n");
        return -1;
    }

    if(data.proxy->socketType == SocketType::tcp)
    {
        data.fd1 = socketConnectNonBlock(data.proxy->address.c_str(), data.proxy->port, connected, log);
    }
    else if(data.proxy->socketType == SocketType::unix)
    {
        data.fd1 = socketConnectUnixNonBlock(data.proxy->address.c_str(), connected, log);
    }
    else
    {
        log->error("invalid socket type\n");
        return -1;
    }

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


ProcessResult ProxyExecutorReadWrite::process(ExecutorData &data, int fd, int events)
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
    if((data.state == ExecutorData::State::forwardResponse ||
            data.state == ExecutorData::State::forwardResponseOnlyWrite) && fd == data.fd0 && (events & EPOLLOUT))
    {
        return process_forwardResponseWrite(data);
    }

    log->warning("invalid process call (proxy)\n");
    return ProcessResult::removeExecutorError;
}


ProcessResult ProxyExecutorReadWrite::process_forwardRequest(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        ssize_t bytesWritten = write(data.fd1, p, size);

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
        log->error("buffer.startRead failed\n");
        return ProcessResult::removeExecutorError;
    }
}


ProcessResult ProxyExecutorReadWrite::process_forwardResponseRead(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startWrite(p, size))
    {
        ssize_t bytesRead = read(data.fd1, p, size);

        log->debug("proxy read bytes: %zd\n", bytesRead);

        if(bytesRead <= 0)
        {
            if(bytesRead == 0)
            {
                loop->closeFd(data, data.fd1);
                data.state = ExecutorData::State::forwardResponseOnlyWrite;
                return process_forwardResponseWrite(data);
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
        else // bytesRead > 0
        {
            data.retryCounter = 0;
            data.buffer.endWrite(bytesRead);
            return process_forwardResponseWrite(data);
        }
    }
    else // no place in buffer
    {
        if(data.pollData1 != nullptr)
        {
            if(loop->removePollFd(data, data.fd1) != 0)
            {
                return ProcessResult::removeExecutorError;
            }
        }

        if(data.pollData0 == nullptr)
        {
            if(loop->addPollFd(data, data.fd0, EPOLLOUT) != 0)
            {
                return ProcessResult::removeExecutorError;
            }
        }

        ++data.retryCounter;
        return ProcessResult::ok;
    }
}


ProcessResult ProxyExecutorReadWrite::process_forwardResponseWrite(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        int errorCode = 0;
        ssize_t bytesWritten = writeFd0(data, p, size, errorCode);

        log->debug("proxy write bytes: %zd\n", bytesWritten);

        if(bytesWritten <= 0)
        {
            if(errorCode == EAGAIN || errorCode == EWOULDBLOCK)
            {
                if(data.pollData0 == nullptr)
                {
                    if(loop->addPollFd(data, data.fd0, EPOLLOUT) != 0)
                    {
                        return ProcessResult::removeExecutorError;
                    }
                }

                ++data.retryCounter;
                return ProcessResult::ok;
            }
            else
            {
                log->error("writeFd0 failed: %s\n", strerror(errorCode));
                return ProcessResult::removeExecutorError;
            }
        }
        else // bytesWritten > 0
        {
            data.retryCounter = 0;
            data.buffer.endRead(bytesWritten);

            if(data.state == ExecutorData::State::forwardResponseOnlyWrite && !data.buffer.readAvailable())
            {
                return ProcessResult::removeExecutorOk;
            }

            if(data.pollData1 == nullptr && data.state == ExecutorData::State::forwardResponse)
            {
                if(loop->addPollFd(data, data.fd1, EPOLLIN) != 0)
                {
                    return ProcessResult::removeExecutorError;
                }
            }

            return ProcessResult::ok;
        }
    }
    else // no data in buffer
    {
        if(data.state == ExecutorData::State::forwardResponseOnlyWrite)
        {
            return ProcessResult::removeExecutorOk;
        }

        if(data.pollData0 != nullptr)
        {
            if(loop->removePollFd(data, data.fd0) != 0)
            {
                return ProcessResult::removeExecutorError;
            }
        }
    }

    ++data.retryCounter;
    return ProcessResult::ok;
}


ProcessResult ProxyExecutorReadWrite::process_waitConnect(ExecutorData &data)
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

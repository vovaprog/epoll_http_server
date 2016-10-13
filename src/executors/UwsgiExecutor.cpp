#include <UwsgiExecutor.h>
#include <PollLoopBase.h>
#include <NetworkUtils.h>

#include <sys/epoll.h>
#include <unistd.h>

int UwsgiExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int UwsgiExecutor::up(ExecutorData &data)
{
    data.removeOnTimeout = true;

    data.fd1 = socketConnect("127.0.0.1", data.port, log);

    if(data.fd1 < 0)
    {
        return -1;
    }

	if(setNonBlock(data.fd1, log) != 0)
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

    data.state = ExecutorData::State::forwardRequest;

    return 0;
}


ProcessResult UwsgiExecutor::process(ExecutorData &data, int fd, int events)
{
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

    loop->log->warning("invalid process call (uwsgi)\n");
    return ProcessResult::removeExecutor;
}


ProcessResult UwsgiExecutor::process_forwardRequest(ExecutorData &data)
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
                return ProcessResult::ok;
            }
            else
            {
                log->error("sendfile failed: %s\n", strerror(errno));
                return ProcessResult::removeExecutor;
            }
        }

        data.buffer.endRead(bytesWritten);

        if(bytesWritten == size)
        {
            data.buffer.clear();

            data.state = ExecutorData::State::forwardResponse;

            if(loop->addPollFd(data, data.fd0, EPOLLOUT) != 0)
            {
                return ProcessResult::removeExecutor;
            }
            if(loop->editPollFd(data, data.fd1, EPOLLIN) != 0)
            {
                return ProcessResult::removeExecutor;
            }
            return ProcessResult::ok;
        }

        return ProcessResult::ok;
    }
    else
    {
        loop->log->error("buffer.startRead failed\n");
        return ProcessResult::removeExecutor;
    }
}


ProcessResult UwsgiExecutor::process_forwardResponseRead(ExecutorData &data)
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
                return ProcessResult::ok;
            }
            else
            {
                log->error("UwsgiExecutor::process_forwardResponseRead   read failed: %s\n", strerror(errno));
                return ProcessResult::removeExecutor;
            }
        }
        else
        {
            data.buffer.endWrite(bytesRead);
            return ProcessResult::ok;
        }
    }
    else
    {
        return ProcessResult::ok;
    }
}


ProcessResult UwsgiExecutor::process_forwardResponseWrite(ExecutorData &data)
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
                return ProcessResult::ok;
            }
            else
            {
                log->error("write failed: %s\n", strerror(errorCode));
                return ProcessResult::removeExecutor;
            }
        }

        data.buffer.endRead(bytesWritten);
    }

    return ProcessResult::ok;
}


ProcessResult UwsgiExecutor::process_forwardResponseOnlyWrite(ExecutorData &data)
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
                return ProcessResult::ok;
            }
            else
            {
                log->error("write failed: %s\n", strerror(errorCode));
                return ProcessResult::removeExecutor;
            }
        }

        data.buffer.endRead(bytesWritten);

        if(bytesWritten == size && !data.buffer.readAvailable())
        {
            return ProcessResult::removeExecutor;
        }
        else
        {
            return ProcessResult::ok;
        }
    }
    else
    {
        return ProcessResult::removeExecutor;
    }
}

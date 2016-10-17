#include <SslFileExecutor.h>
#include <PollLoopBase.h>
#include <SslUtils.h>

#include <sys/epoll.h>
#include <unistd.h>


ProcessResult SslFileExecutor::process(ExecutorData &data, int fd, int events)
{
    if(data.state == ExecutorData::State::sendFile && fd == data.fd0 && ((events & EPOLLIN) || (events & EPOLLOUT)))
    {
        return process_sendFile(data);
    }
    else
    {
        return FileExecutor::process(data, fd, events);
    }
}


ProcessResult SslFileExecutor::process_sendFile(ExecutorData &data)
{
    void *p;
    int size;
    bool operationOk = false;

    if(data.fd1 > 0)
    {
        if(data.buffer.startWrite(p, size))
        {
            int bytesRead = read(data.fd1, p, size);

            if(bytesRead < 0)
            {
                return ProcessResult::removeExecutor;
            }
            else if(bytesRead == 0)
            {
                close(data.fd1);
                data.fd1 = -1;
            }
            else
            {
                operationOk = true;
                data.buffer.endWrite(bytesRead);
            }
        }
    }
    if(data.buffer.startRead(p, size))
    {
        int bytesWritten = SSL_write(data.ssl, p, size);

        if(bytesWritten > 0)
        {
            operationOk = true;
            data.buffer.endRead(bytesWritten);

            data.bytesToSend -= bytesWritten;

            if(data.bytesToSend == 0)
            {
                return ProcessResult::removeExecutor;
            }
        }
        else
        {
            int error = SSL_get_error(data.ssl, bytesWritten);

            if(error == SSL_ERROR_WANT_WRITE)
            {
                return ProcessResult::ok;
            }
            else if(error == SSL_ERROR_WANT_READ)
            {
                if(loop->editPollFd(data, data.fd0, EPOLLIN | EPOLLOUT) != 0)
                {
                    return ProcessResult::removeExecutor;
                }
            }
            else
            {
                log->error("SSL_write failed: %d\n", error);
                return ProcessResult::removeExecutor;
            }
        }
    }
    else
    {
        if(data.fd1 > 0)
        {
            log->error("buffer.startRead failed\n");
            return ProcessResult::removeExecutor;
        }
    }

    if(operationOk)
    {
        data.badOperationCounter = 0;
    }
    else
    {
        ++data.badOperationCounter;
    }

    return ProcessResult::ok;
}


ssize_t SslFileExecutor::writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode)
{
    return sslWriteFd0(this, data, buf, count, errorCode);
}


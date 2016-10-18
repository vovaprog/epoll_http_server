#include <PollLoopBase.h>
#include <FileExecutor.h>
#include <FileUtils.h>
#include <HttpUtils.h>

#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <algorithm>


int FileExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int FileExecutor::up(ExecutorData &data)
{
    data.removeOnTimeout = true;

    data.fd1 = open(loop->fileNameBuffer, O_NONBLOCK | O_RDONLY);

    if(data.fd1 < 0)
    {
        log->warning("open failed. file: %s   error: %s\n", loop->fileNameBuffer, strerror(errno));

        if(errno == ENOENT)
        {
            if(createError(data, HttpCode::notFound) != 0)
            {
                return -1;
            }
            if(loop->editPollFd(data, data.fd0, EPOLLOUT) != 0)
            {
                return -1;
            }
            data.state = ExecutorData::State::sendError;
            return 0;
        }

        return -1;
    }

    data.bytesToSend = fileSize(data.fd1);

    if(data.bytesToSend < 0)
    {
        log->error("fileSize failed: %s\n", strerror(errno));
        return -1;
    }

    if(createResponse(data) != 0)
    {
        return -1;
    }

    if(loop->editPollFd(data, data.fd0, EPOLLOUT) != 0)
    {
        return -1;
    }

    data.state = ExecutorData::State::sendResponse;

    return 0;
}


ProcessResult FileExecutor::process(ExecutorData &data, int fd, int events)
{
    if(data.state == ExecutorData::State::sendResponse && fd == data.fd0 && (events & EPOLLOUT))
    {
        return process_sendResponseSendData(data);
    }
    if(data.state == ExecutorData::State::sendFile && fd == data.fd0 && (events & EPOLLOUT))
    {
        return process_sendFile(data);
    }
    if(data.state == ExecutorData::State::sendError && fd == data.fd0 && (events & EPOLLOUT))
    {
        return process_sendResponseSendData(data);
    }

    log->warning("invalid process call (file)\n");
    return ProcessResult::removeExecutor;
}


int FileExecutor::createResponse(ExecutorData &data)
{
    data.buffer.clear();

    void *p;
    int size;

    if(data.buffer.startWrite(p, size))
    {
        int ret = snprintf((char*)p, size, "HTTP/1.1 200 Ok\r\nContent-Length: %lld\r\nConnection: close\r\n\r\n", data.bytesToSend);
        if(ret < 0)
        {
            return -1;
        }
        size = std::min(size, ret);
        data.buffer.endWrite(size);
        return 0;
    }
    else
    {
        log->warning("buffer.startWrite failed\n");
        return -1;
    }
}


int FileExecutor::createError(ExecutorData &data, int statusCode)
{
    data.buffer.clear();

    void *p;
    int size;

    const char *statusString = httpCode2String((int)statusCode);

    if(statusString == nullptr)
    {
        return -1;
    }

    if(data.buffer.startWrite(p, size))
    {
        int ret = snprintf((char*)p, size, "HTTP/1.1 %d %s\r\nConnection: close\r\n\r\n"
                           "<html><head>"
                           "<title>404 Not Found</title>"
                           "</head><body>"
                           "<h1>Not Found</h1>"
                           "<p>The requested URL was not found on this server.</p>"
                           "</body></html>", (int)statusCode, statusString);
        if(ret < 0)
        {
            return -1;
        }
        size = std::min(size, ret);
        data.buffer.endWrite(size);
        return 0;
    }
    else
    {
        log->warning("buffer.startWrite failed\n");
        return -1;
    }
}


ProcessResult FileExecutor::process_sendResponseSendData(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        int errorCode = 0;
        int bytesWritten = writeFd0(data, p, size, errorCode);

        if(bytesWritten <= 0)
        {
            if(errorCode != EWOULDBLOCK && errorCode != EAGAIN && errorCode != EINTR)
            {
                return ProcessResult::removeExecutor;
            }
            else
            {
                ++data.retryCounter;
            }
        }
        else
        {
            data.retryCounter = 0;
            data.buffer.endRead(bytesWritten);

            if(bytesWritten == size)
            {
                data.buffer.clear();

                if(data.state == ExecutorData::State::sendError)
                {
                    return ProcessResult::removeExecutor;
                }
                else
                {
                    data.state = ExecutorData::State::sendFile;
                }
            }
        }

        return ProcessResult::ok;
    }
    else
    {
        return ProcessResult::removeExecutor;
    }
}


ProcessResult FileExecutor::process_sendFile(ExecutorData &data)
{
    int bytesWritten = sendfile(data.fd0, data.fd1, &data.filePosition, data.bytesToSend);
    if(bytesWritten <= 0)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            ++data.retryCounter;
            return ProcessResult::ok;
        }
        else
        {
            log->error("sendfile failed: %s\n", strerror(errno));
            return ProcessResult::removeExecutor;
        }

    }

    data.retryCounter = 0;

    data.bytesToSend -= bytesWritten;

    if(data.bytesToSend == 0)
    {
        return ProcessResult::removeExecutor;
    }

    return ProcessResult::ok;
}


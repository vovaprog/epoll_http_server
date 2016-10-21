#include <RequestExecutor2.h>

#include <UwsgiApplicationParameters.h>
#include <PollLoopBase.h>

#include <percent_decode.h>
#include <sys/epoll.h>

int RequestExecutor2::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int RequestExecutor2::up(ExecutorData &data)
{
    log->debug("RequestExecutor up called\n");

    data.removeOnTimeout = true;
    data.connectionType = (int)ConnectionType::clear;

    data.buffer.init(ExecutorData::REQUEST_BUFFER_SIZE);

    data.request.reset();

    if(loop->addPollFd(data, data.fd0, EPOLLIN) != 0)
    {
        return -1;
    }

    data.state = ExecutorData::State::readRequest;

    return 0;
}


ProcessResult RequestExecutor2::process(ExecutorData &data, int fd, int events)
{
    if(data.state == ExecutorData::State::readRequest && fd == data.fd0 && (events & EPOLLIN))
    {
        return process_readRequest(data);
    }

    log->warning("invalid process call (request)\n");
    return ProcessResult::removeExecutorError;
}


int RequestExecutor2::readRequest(ExecutorData &data)
{
    void *p;
    int size;
    bool readOk = false;

    if(data.buffer.startWrite(p, size))
    {
        int errorCode = 0;
        int rd = readFd0(data, p, size, errorCode);

        if(rd <= 0)
        {
            if(errorCode != EWOULDBLOCK && errorCode != EAGAIN && errorCode != EINTR)
            {
                if(rd == 0 && errorCode == 0)
                {
                    log->debug("client disconnected\n");
                }
                else
                {
                    log->warning("RequestExecutor::readRequest   read failed: %s   (%d)\n", strerror(errorCode), errorCode);
                }
                return -1;
            }
        }
        else
        {
            readOk = true;
            data.buffer.endWrite(rd);
        }
    }
    else
    {
        log->warning("requestBuffer.startWrite failed");
        return -1;
    }

    if(readOk)
    {
        data.retryCounter = 0;
    }
    else
    {
        ++data.retryCounter;
    }

    return 0;
}


ProcessResult RequestExecutor2::setExecutor(ExecutorData &data, Executor *pExecutor)
{
    data.pExecutor = pExecutor;
    if(pExecutor->up(data) != 0)
    {
        log->warning("RequestExecutor::setExecutor up failed\n");
        return ProcessResult::removeExecutorError;
    }
    else
    {
        return ProcessResult::ok;
    }
}


int RequestExecutor2::findApplicationIndex(ExecutorData &data)
{
    int size = loop->parameters->uwsgiApplications.size();
    for(int i = 0; i < size ;++i)
    {
        if(data.request.isUrlPrefix(loop->parameters->uwsgiApplications[i].prefix.c_str()))
        {
            return i;
        }
    }

    return -1;
}


RequestExecutor2::ParseRequestResult RequestExecutor2::parseRequest(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        HttpRequest::ParseResult result = data.request.parse(static_cast<char*>(p), size);

        if(result == HttpRequest::ParseResult::finishOk)
        {
            data.request.print();

            int appIndex = findApplicationIndex(data);

            if(appIndex >= 0)
            {
                UwsgiApplicationParameters &app = loop->parameters->uwsgiApplications[appIndex];

                if((data.connectionType & app.connectionType) == 0)
                {
                    return ParseRequestResult::invalid;
                }

                data.port = app.port;
                return ParseRequestResult::uwsgi;
            }
            else
            {
                const char *url = data.request.getUrl();

                if(url == nullptr)
                {
                    return ParseRequestResult::invalid;
                }

                loop->fileNameBuffer[loop->rootFolderLength] = 0;

                if(strcmp(url, "/") == 0)
                {
                    strcat(loop->fileNameBuffer, "index.html");
                }
                else
                {
                    strcat(loop->fileNameBuffer, url);
                }

                return ParseRequestResult::file;
            }
        }
        else if(result == HttpRequest::ParseResult::needMoreData)
        {
            return ParseRequestResult::again;
        }
        else
        {
            return ParseRequestResult::invalid;
        }
    }

    return RequestExecutor2::ParseRequestResult::again;
}


ProcessResult RequestExecutor2::process_readRequest(ExecutorData &data)
{
    if(readRequest(data) != 0)
    {
        return ProcessResult::removeExecutorError;
    }

    ParseRequestResult parseResult = parseRequest(data);

    if(parseResult == ParseRequestResult::file)
    {
        return setExecutor(data, loop->getExecutor(ExecutorType::file));
    }
    else if(parseResult == ParseRequestResult::uwsgi)
    {
        return setExecutor(data, loop->getExecutor(ExecutorType::uwsgi));
    }
    else if(parseResult == ParseRequestResult::invalid)
    {
        return ProcessResult::removeExecutorError;
    }

    return ProcessResult::ok;
}

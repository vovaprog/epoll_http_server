#include <RequestExecutor.h>

#include <UwsgiApplicationParameters.h>
#include <PollLoopBase.h>

#include <sys/epoll.h>

int RequestExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int RequestExecutor::up(ExecutorData &data)
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


ProcessResult RequestExecutor::process(ExecutorData &data, int fd, int events)
{
    if(data.state == ExecutorData::State::readRequest && fd == data.fd0 && (events & EPOLLIN))
    {
        return process_readRequest(data);
    }

    log->warning("invalid process call (request)\n");
    return ProcessResult::removeExecutorError;
}


int RequestExecutor::readRequest(ExecutorData &data)
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


ProcessResult RequestExecutor::setExecutor(ExecutorData &data, Executor *pExecutor)
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


int RequestExecutor::findApplicationIndex(ExecutorData &data)
{
    int size = loop->parameters->uwsgiApplications.size();
    for(int i = 0; i < size ; ++i)
    {
        if(data.request.isUrlPrefix(loop->parameters->uwsgiApplications[i].prefix.c_str()))
        {
            return i;
        }
    }

    return -1;
}


RequestExecutor::ParseRequestResult RequestExecutor::parseRequest(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        {
            char *cp = static_cast<char*>(p);
            char tempChar = cp[size - 1];
            cp[size - 1] = 0;

            log->debug("request:\n[[[[[%s]]]]]\n", cp);

            cp[size - 1] = tempChar;
        }


        HttpRequest::ParseResult result = data.request.parse(static_cast<char*>(p), size);

        if(result == HttpRequest::ParseResult::finishOk)
        {
            log->debug("url: [%s]\n", data.request.getUrl());

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
                    int urlLength = strlen(url);

                    if(loop->rootFolderLength + urlLength <= PollLoopBase::MAX_FILE_NAME)
                    {
                        strcat(loop->fileNameBuffer, url);
                    }
                    else
                    {
                        return ParseRequestResult::invalid;
                    }
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
            char *cp = static_cast<char*>(p);
            char tempChar = cp[size - 1];
            cp[size - 1] = 0;

            log->info("invalid request:\n%s\n", cp);

            cp[size - 1] = tempChar;

            return ParseRequestResult::invalid;
        }
    }

    return RequestExecutor::ParseRequestResult::again;
}


ProcessResult RequestExecutor::process_readRequest(ExecutorData &data)
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

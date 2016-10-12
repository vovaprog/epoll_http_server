#include <RequestExecutor.h>

#include <UwsgiApplicationParameters.h>
#include <PollLoopBase.h>

#include <percent_decode.h>
#include <sys/epoll.h>

int RequestExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    this->log = loop->log;
    return 0;
}


int RequestExecutor::up(ExecutorData &data)
{
    data.removeOnTimeout = true;

    data.buffer.init(ExecutorData::REQUEST_BUFFER_SIZE);

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
    return ProcessResult::removeExecutor;
}


int RequestExecutor::readRequest(ExecutorData &data)
{
    void *p;
    int size;

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
                    log->info("RequestExecutor::readRequest   read failed: %s\n", strerror(errorCode));
                }
                return -1;
            }
        }
        else
        {
            data.buffer.endWrite(rd);
        }
    }
    else
    {
        log->warning("requestBuffer.startWrite failed");
        return -1;
    }

    return 0;
}


int RequestExecutor::checkApplicationUrl(ExecutorData &data, char *urlBuffer)
{
    char prevChar = 0;
    int i;
    for(i = 0; i < ExecutorData::REQUEST_BUFFER_SIZE && urlBuffer[i] != 0 ; ++i)
    {
        if(!((urlBuffer[i] >= 'a' && urlBuffer[i] <= 'z') ||
                (urlBuffer[i] >= 'A' && urlBuffer[i] <= 'Z') ||
                (urlBuffer[i] >= '0' && urlBuffer[i] <= '9') ||
                urlBuffer[i] == '/' || urlBuffer[i] == '.' || urlBuffer[i] == '_' ||
                urlBuffer[i] == '=' || urlBuffer[i] == '?' || urlBuffer[i] == '-' || urlBuffer[i] == ' '))
        {
            return -1;
        }
        if(urlBuffer[i] == '.' && urlBuffer[i] == prevChar)
        {
            return -1;
        }
        prevChar = urlBuffer[i];
    }
    if(loop->rootFolderLength + i > PollLoopBase::MAX_FILE_NAME)
    {
        return -1;
    }
    return 0;
}


int RequestExecutor::checkFileUrl(ExecutorData &data, char *urlBuffer)
{
    char prevChar = 0;
    int i;
    for(i = 0; i < ExecutorData::REQUEST_BUFFER_SIZE && urlBuffer[i] != 0 ; ++i)
    {
        if(!((urlBuffer[i] >= 'a' && urlBuffer[i] <= 'z') ||
                (urlBuffer[i] >= 'A' && urlBuffer[i] <= 'Z') ||
                (urlBuffer[i] >= '0' && urlBuffer[i] <= '9') ||
                urlBuffer[i] == '/' || urlBuffer[i] == '.' || urlBuffer[i] == '_' ||
                urlBuffer[i] == '-' || urlBuffer[i] == ' '))
        {
            return -1;
        }
        if(urlBuffer[i] == '.' && urlBuffer[i] == prevChar)
        {
            return -1;
        }
        prevChar = urlBuffer[i];
    }
    if(loop->rootFolderLength + i > PollLoopBase::MAX_FILE_NAME)
    {
        return -1;
    }
    return 0;
}


bool RequestExecutor::isUrlPrefix(const char *url, const char *prefix)
{
    int ui, pi;
    for(ui = 0; url[ui] == '/'; ++ui);
    for(pi = 0; prefix[pi] == '/'; ++pi);

    for(; url[ui] != 0 && prefix[pi] != 0 && url[ui] == prefix[pi]; ++ui, ++pi);

    if((prefix[pi] == 0 || prefix[pi] == '/') && (url[ui] == 0 || url[ui] == '/' || url[ui] == '?'))
    {
        return true;
    }

    return false;
}


RequestExecutor::ParseRequestResult RequestExecutor::parseRequest(ExecutorData &data)
{
    void *p;
    int size;

    if(data.buffer.startRead(p, size))
    {
        if(size > 1)
        {
            char *cdata = (char*)p;

            char *endOfHeaders = strstr(cdata, "\r\n\r\n");

            if(endOfHeaders != nullptr)
            {
                if(size < ExecutorData::REQUEST_BUFFER_SIZE)
                {
                    cdata[size] = 0;
                }
                else
                {
                    return ParseRequestResult::invalid;
                }

                char urlBuffer[ExecutorData::REQUEST_BUFFER_SIZE];

                if(sscanf(cdata, "GET %s HTTP", urlBuffer) != 1)
                {
                    if(sscanf(cdata, "POST %s HTTP", urlBuffer) != 1)
                    {
                        log->info("invalid url\n");
                        return ParseRequestResult::invalid;
                    }
                }

                if(percent_decode_in_place(urlBuffer, 1) == nullptr)
                {
                    return ParseRequestResult::invalid;
                }

                bool isUwsgiApplication = false;

                for(UwsgiApplicationParameters& app : loop->parameters->uwsgiApplications)
                {
                    if(isUrlPrefix(urlBuffer, app.prefix.c_str()))
                    {
                        data.port = app.port;
                        isUwsgiApplication = true;
                        break;
                    }
                }

                if(isUwsgiApplication)
                {
                    if(checkApplicationUrl(data, urlBuffer) != 0)
                    {
                        log->info("invalid url\n");
                        return ParseRequestResult::invalid;
                    }
                }
                else
                {
                    if(checkFileUrl(data, urlBuffer) != 0)
                    {
                        log->info("invalid url\n");
                        return ParseRequestResult::invalid;
                    }
                }

                log->debug("url: %s\n", urlBuffer);

                loop->fileNameBuffer[loop->rootFolderLength] = 0;

                if(strcmp(urlBuffer, "/") == 0)
                {
                    strcat(loop->fileNameBuffer, "index.html");
                }
                else
                {
                    strcat(loop->fileNameBuffer, urlBuffer);
                }

                if(isUwsgiApplication)
                {
                    return ParseRequestResult::uwsgi;
                }
                else
                {
                    return ParseRequestResult::file;
                }
            }
        }
    }
    return ParseRequestResult::again;
}


ProcessResult RequestExecutor::setExecutor(ExecutorData &data, Executor *pExecutor)
{
    data.pExecutor = pExecutor;
    if(pExecutor->up(data) != 0)
    {
        return ProcessResult::removeExecutor;
    }
    else
    {
        return ProcessResult::ok;
    }
}


ProcessResult RequestExecutor::process_readRequest(ExecutorData &data)
{
    if(readRequest(data) != 0)
    {
        return ProcessResult::removeExecutor;
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
        return ProcessResult::removeExecutor;
    }

    return ProcessResult::ok;
}


#include <HttpRequest.h>
#include <HttpUtils.h>
#include <TimeUtils.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


HttpRequest::ParseResult HttpRequest::startParse(const char *data, int size)
{
    reset();

    this->data = data;
    this->size = size;
    cur = 0;
    state = State::method;

    ParseResult result = parse();

    if(result != ParseResult::finishOk)
    {
        return result;
    }

    return postParse();
}

HttpRequest::ParseResult HttpRequest::continueParse(const char *data, int size)
{
    this->data = data;
    this->size = size;

    ParseResult result = parse();

    if(result != ParseResult::finishOk)
    {
        return result;
    }

    return postParse();
}


HttpRequest::ParseResult HttpRequest::postParse()
{
    if(decodeUrl() != 0)
    {
        state = State::invalid;
        return ParseResult::finishInvalid;
    }
    return ParseResult::finishOk;
}


const char* HttpRequest::getUrl()
{
    if(state != State::finishOk)
    {
        return nullptr;
    }

    return urlBuffer;
}


int HttpRequest::decodeUrl()
{
    if(state != State::finishOk)
    {
        return -1;
    }

    if(urlLength > 0)
    {
        if(urlBufferSize < urlLength + 1)
        {
            delete[] urlBuffer;
            urlBuffer = new char[urlLength + 1];
        }
        if(percentDecode(data + urlStart, urlBuffer, urlLength) != 0)
        {
            return -1;
        }
        return 0;
    }
    else
    {
        return -1;
    }
}


int HttpRequest::getHeaderValue(const char *key, const char **ptr, int *size)
{
    if(state != State::finishOk)
    {
        return -1;
    }

    for(Header &head : headers)
    {
        if(strncasecmp(key, data + head.key.start, head.key.length) == 0)
        {
            *ptr = data + head.value.start;
            *size = head.value.length;
            return 0;
        }
    }

    return -1;
}


time_t HttpRequest::getIfModifiedSince()
{
    const char *ptr;
    int length;
    if(getHeaderValue("If-Modified-Since", &ptr, &length) == 0)
    {
        char buf[101];
        if(length > 100)
        {
            return 0;
        }
        strncpy(buf, ptr, length);

        struct tm time_data;
        if(strptime(buf, RFC1123FMT, &time_data) == NULL)
        {
            return 0;
        }
        time_t result = timegm(&time_data);

        return result;
    }
    return 0;
}


bool HttpRequest::isUrlPrefix(const char *prefix)
{
    if(state != State::finishOk)
    {
        return false;
    }

    int ui, pi;
    for(ui = 0; urlBuffer[ui] == '/'; ++ui);
    for(pi = 0; prefix[pi] == '/'; ++pi);

    for(; urlBuffer[ui] != 0 && prefix[pi] != 0 && urlBuffer[ui] == prefix[pi]; ++ui, ++pi);

    if((prefix[pi] == 0 || prefix[pi] == '/') && (urlBuffer[ui] == 0 || urlBuffer[ui] == '/' || urlBuffer[ui] == '?'))
    {
        return true;
    }
    if(pi > 0 && ui > 0 && prefix[pi - 1] == '/' && urlBuffer[ui - 1] == '/')
    {
        return true;
    }

    return false;
}


int HttpRequest::print()
{
    if(methodLength > 0)
    {
        printf("method: [%.*s]\n", methodLength, data + methodStart);
    }
    if(urlLength > 0)
    {
        printf("url: [%.*s]\n", urlLength, data + urlStart);
    }
    if(urlParametersLength > 0)
    {
        printf("url params: [%.*s]\n", urlParametersLength, data + urlParametersStart);
    }
    for(const Header &head : headers)
    {
        printf("[%.*s]: [%.*s]\n", head.key.length, data + head.key.start, head.value.length, data + head.value.start);
    }
    if(contentLength > 0)
    {
        printf("content (%d): [%.*s]\n", contentLength, contentLength, data + contentStart);
    }

    return 0;
}


HttpRequest::ReadResult HttpRequest::readMethod(int &length)
{
    int i = cur;

    while(i < size)
    {
        if(data[i] >= 'A' && data[i] <= 'Z')
        {
            ++i;
        }
        else if(data[i] == ' ')
        {
            length = i - cur;
            return ReadResult::ok;
        }
        else
        {
            return ReadResult::invalid;
        }
    }
    return ReadResult::needMoreData;
}


HttpRequest::ReadResult HttpRequest::readSpaces(int &length)
{
    int i;

    for(i = cur; i < size && data[i] == ' '; ++i);

    if(i == size)
    {
        return ReadResult::needMoreData;
    }
    else if(i == '\r' || i == '\n')
    {
        return ReadResult::invalid;
    }
    else
    {
        length = i - cur;
        return ReadResult::ok;
    }
}


HttpRequest::ReadResult HttpRequest::readUrl(int &length)
{
    int i = cur;

    while(i < size)
    {
        if((data[i] >= 'a' && data[i] <= 'z') ||
                (data[i] >= 'A' && data[i] <= 'Z') ||
                (data[i] >= '0' && data[i] <= '9') ||
                data[i] == '/' || data[i] == '.' || data[i] == '_' ||
                data[i] == '=' || data[i] == '-' || data[i] == '%')
        {
            ++i;
        }
        else if(data[i] == ' ')
        {
            length = i - cur;
            return ReadResult::ok;
        }
        else if(data[i] == '?')
        {
            length = i - cur;
            return ReadResult::question;
        }
        else
        {
            return ReadResult::invalid;
        }
    }
    return ReadResult::needMoreData;
}


HttpRequest::ReadResult HttpRequest::readUrlParameters(int &length)
{
    int i;

    for(i = cur; i < size && data[i] != ' '; ++i);

    if(i == size)
    {
        return ReadResult::needMoreData;
    }
    else if(data[i] == '\r' || data[i] == '\n')
    {
        return ReadResult::invalid;
    }
    else
    {
        length = i - cur;
        return ReadResult::ok;
    }
}


HttpRequest::ReadResult HttpRequest::readToNextLine(int &length)
{
    int i;

    for(i = cur; i < size && data[i] != '\r' && data[i] != '\n'; ++i);

    if(i == size)
    {
        return ReadResult::needMoreData;
    }

    int lineBreakCounter = 0;
    for(; i < size && (data[i] == '\r' || data[i] == '\n'); ++i)
    {
        if(data[i] == '\n')
        {
            ++lineBreakCounter;
        }
    }

    if(lineBreakCounter > 1)
    {
        return ReadResult::endOfHeaders;
    }

    if(i == size)
    {
        return ReadResult::needMoreData;
    }

    length = i - cur;
    return ReadResult::ok;
}


HttpRequest::ReadResult HttpRequest::readHeaderKey(int &length)
{
    int i = cur;

    while(i < size)
    {
        if((data[i] >= 'A' && data[i] <= 'Z') ||
                (data[i] >= 'a' && data[i] <= 'z') ||
                data[i] == '-')
        {
            ++i;
        }
        else if(data[i] == ':')
        {
            length = i - cur;
            return ReadResult::ok;
        }
        else
        {
            return ReadResult::invalid;
        }
    }
    return ReadResult::needMoreData;
}


HttpRequest::ReadResult HttpRequest::readHeaderSpace(int &length)
{
    int i = cur;

    while(i < size)
    {
        if(data[i] == ':' || data[i] == ' ')
        {
            ++i;
        }
        else if(data[i] == '\r' || data[i] == '\n')
        {
            return ReadResult::invalid;
        }
        else
        {
            length = i - cur;
            return ReadResult::ok;
        }
    }
    return ReadResult::needMoreData;
}


HttpRequest::ReadResult HttpRequest::readHeaderValue(int &length)
{
    int i = cur;

    while(i < size)
    {
        if(data[i] != '\r' && data[i] != '\n')
        {
            ++i;
        }
        else
        {
            length = i - cur;
            return ReadResult::ok;
        }
    }
    return ReadResult::needMoreData;
}


HttpRequest::ReadResult HttpRequest::readHeaderLineBreak(int &length)
{
    int i = cur;

    int lineBreakCounter = 0;
    for(; i < size && (data[i] == '\r' || data[i] == '\n'); ++i)
    {
        if(data[i] == '\n')
        {
            ++lineBreakCounter;
        }
    }

    if(lineBreakCounter > 1)
    {
        length = i - cur;
        return ReadResult::endOfHeaders;
    }

    if(i == size)
    {
        return ReadResult::needMoreData;
    }

    length = i - cur;
    return ReadResult::ok;
}


HttpRequest::ParseResult HttpRequest::parse()
{
    ReadResult result;
    int length;

    while(true)
    {
        if(state == State::method)
        {
            result = readMethod(length);
            if(result == ReadResult::ok)
            {
                methodStart = cur;
                methodLength = length;

                cur += length;
                state = State::spaceAfterMethod;
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::spaceAfterMethod)
        {
            result = readSpaces(length);

            if(result == ReadResult::ok)
            {
                cur += length;
                state = State::url;
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::url)
        {
            result = readUrl(length);

            if(result == ReadResult::ok || result == ReadResult::question)
            {
                urlStart = cur;
                urlLength = length;

                cur += length;
                if(result == ReadResult::ok)
                {
                    state = State::spaceAfterUrl;
                }
                else
                {
                    state = State::urlParameters;
                }
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::urlParameters)
        {
            result = readUrlParameters(length);

            if(result == ReadResult::ok)
            {
                urlParametersStart = cur;
                urlParametersLength = length;

                cur += length;
                state = State::spaceAfterUrl;
            }
        }
        else if(state == State::spaceAfterUrl)
        {
            result = readToNextLine(length);

            if(result == ReadResult::ok)
            {
                cur += length;
                state = State::headerKey;
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::headerKey)
        {
            result = readHeaderKey(length);

            if(result == ReadResult::ok)
            {
                curKey.start = cur;
                curKey.length = length;

                cur += length;
                state = State::headerSpace;
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::headerSpace)
        {
            result = readHeaderSpace(length);

            if(result == ReadResult::ok)
            {
                cur += length;
                state = State::headerValue;
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::headerValue)
        {
            result = readHeaderValue(length);

            if(result == ReadResult::ok)
            {
                Header h;
                h.key = curKey;
                h.value.start = cur;
                h.value.length = length;
                headers.push_back(h);

                if(strncasecmp("Content-Length", data + h.key.start, h.key.length) == 0)
                {
                    const int BUF_CHARS = 30;

                    if(h.value.length > BUF_CHARS)
                    {
                        return ParseResult::finishInvalid;
                    }

                    char buf[BUF_CHARS + 1];
                    strncpy(buf, data + h.value.start, h.value.length);

                    contentLength = atoi(buf);
                }

                cur += length;
                state = State::headerLineBreak;
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::headerLineBreak)
        {
            result = readHeaderLineBreak(length);

            if(result == ReadResult::ok)
            {
                cur += length;
                state = State::headerKey;
            }
            else if(result == ReadResult::endOfHeaders)
            {
                if(contentLength == 0)
                {
                    state = State::finishOk;
                    return ParseResult::finishOk;
                }
                else
                {
                    cur += length;
                    state = State::content;
                }
            }
            else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
            else return ParseResult::finishInvalid;
        }
        else if(state == State::content)
        {
            if(size - cur >= contentLength)
            {
                contentStart = cur;
                state = State::finishOk;
                return ParseResult::finishOk;
            }
            else
            {
                return ParseResult::needMoreData;
            }
        }
        else if(state == State::finishOk)
        {
            return ParseResult::finishOk;
        }
        else
        {
            return ParseResult::finishInvalid;
        }
    }
}



#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <vector>

class HttpRequest
{
public:
    enum class ParseResult
    {
        finishOk, finishInvalid, needMoreData
    };


    ParseResult startParse(const char *data, int size)
    {
        reset();

        this->data = data;
        this->size = size;
        cur = 0;
        state = State::method;

        return parse();
    }

    ParseResult continueParse(const char *data, int size)
    {
        this->data = data;
        this->size = size;

        return parse();
    }

    int print()
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

        return 0;
    }

protected:

    enum class State
    {
        method, spaceAfterMethod, url, urlParameters, spaceAfterUrl, headerKey, headerSpace, headerValue, headerLineBreak
    };
    enum class ReadResult
    {
        invalid, needMoreData, ok, question, endOfHeaders
    };


    void reset()
    {
        cur = 0;
        data = nullptr;
        size = 0;
        state = State::method;

        methodStart = 0;
        methodLength = 0;

        urlStart = 0;
        urlLength = 0;

        urlParametersStart = 0;
        urlParametersLength = 0;

        curKey.start = 0;
        curKey.length = 0;

        headers.clear();
    }


    ReadResult readMethod(int &length)
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

    ReadResult readSpaces(int &length)
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

    ReadResult readUrl(int &length)
    {
        int i = cur;

        while(i < size)
        {
            if((data[i] >= 'a' && data[i] <= 'z') ||
                    (data[i] >= 'A' && data[i] <= 'Z') ||
                    (data[i] >= '0' && data[i] <= '9') ||
                    data[i] == '/' || data[i] == '.' || data[i] == '_' ||
                    data[i] == '=' || data[i] == '-')
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

    ReadResult readUrlParameters(int &length)
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

    ReadResult readToNextLine(int &length)
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

    ReadResult readHeaderKey(int &length)
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

    ReadResult readHeaderSpace(int &length)
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

    ReadResult readHeaderValue(int &length)
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

    ReadResult readHeaderLineBreak(int &length)
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
            return ReadResult::endOfHeaders;
        }

        if(i == size)
        {
            return ReadResult::needMoreData;
        }

        length = i - cur;
        return ReadResult::ok;
    }

    ParseResult parse()
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
                    return ParseResult::finishOk;
                }
                else if(result == ReadResult::needMoreData) return ParseResult::needMoreData;
                else return ParseResult::finishInvalid;
            }
        }
    }    

    int cur = 0;
    const char *data = nullptr;
    int size = 0;
    State state = State::method;

    int methodStart = 0;
    int methodLength = 0;

    int urlStart = 0;
    int urlLength = 0;

    int urlParametersStart = 0;
    int urlParametersLength = 0;

    struct Field
    {
        int start = 0;
        int length = 0;
    };

    struct Header
    {
        Field key;
        Field value;
    };

    Field curKey;

    std::vector<Header> headers;
};

#endif // HTTP_REQUEST_H


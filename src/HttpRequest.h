#include <vector>

class HttpRequest {
public:
    enum class ParseResult { finishOk, finishInvalid, needMoreData };


    ParseResult startParse(char *data, int size)
    {
        this->data = data;
        this->size = size;
        cur = 0;
        state = State::method;
    }

    ParseResult continueParse(int size);

protected:

    enum class State { method };
    enum class CharClass { letters };
    enum class ReadResult { invalid, size, space, endOfLine };

    ReadResult readData(CharClass stopCharClass, CharClass checkCharClass, int &length)
    {
        int i = cur;
        while(i < size)
        {
            if(stopCharClass == CharClass::space)
            {
                if(data[i]==' ' || data[i]='\r' || data[i]=='\n')
                {
                    break;
                }
            }
            else if(stopCharClass == CharClass::spaceAndQuestion)
            {

            }

            if(checkCharClass == CharClass::letters &&
              !((data[i]>='A' && data[i]<='Z') || (data[i]>='a' && data[i]<='z')))
            {
                return ReadResult::invalid;
            }
            ++i;
        }
        length = cur - i;
        if(i == size)
        {
            return ReadResult::size;
        }
        else if(data[i] == ' ')
        {
            return ReadResult::space;
        }
        else if(data[i] == '\r' || data[i] == '\n')
        {
            return ReadResult::endOfLine;
        }
        return ReadResult::invalid;
    }

    ParseResult parse()
    {
        ReadResult result;
        int length;

        if(state == State::method)
        {
            result = readTillSpace(CharClass::letters, length);
            if(result == ReadResult::space)
            {
                methodStart = cur;
                methodLength = length;

                cur += length;

                result = readSpaces();
                if(result == ReadResult::word)
                {
                    state = State::url;
                }
                else if(result == ReadResult::size)
                {
                    return ParseResult::needMoreData;
                }
                else
                {
                    return ParseResult::finishInvalid;
                }
            }
            else if(result == size)
            {
                return ParseResult::needMoreData;
            }
            else
            {
                return ParseResult::finishInvalid;
            }
        }
        else if(state == State::url)
        {

            result = readData(CharClass::spaceAndQuestion, CharClass::url, length);
            if(result == ReadResult::space || result == ReadResult::question)
            {
                urlStart = cur;
                urlLength = length;

                cur += length;

                if(result == ReadResult::space)
                {
                    result = readToNextLine();
                    if(result == ReadResult::word)
                    {
                        state = State::headerKey;
                    }
                    else if(result == ReadResult::size)
                    {
                        return ParseResult::needMoreData;
                    }
                    else
                    {
                        return ParseResult::finishInvalid;
                    }
                }
                else if(result == ReadResult::question)
                {
                    state = State::urlParameters;
                }
            }
            else if(result == ReadResult::size)
            {
                return ParseResult::needMoreData;
            }
            else
            {
                return ParseResult::finishInvalid;
            }
        }
        else if(state == State::urlParameters)
        {
            result = readData(CharClass::space, CharClass::url, length);

            if(result == ReadResult::space)
            {
                urlParametersStart = cur;
                urlParametersLength = length;

                result = readToNextLine();
                if(result == ReadResult::word)
                {
                    state = State::headerKey;
                }
                else if(result == ReadResult::size)
                {
                    return ParseResult::needMoreData;
                }
                else
                {
                    return ParseResult::finishInvalid;
                }
            }
        }
        else if(state == State::headerKey)
        {
            result = readData(CharClass::colon, CharClass::headerKey, length);
            if(result == CharClass::colon)
            {
                curKey.start = cur;
                curKey.length = length;

                int i;
                for(i=0;i<size && (data[i]==':' || data[i] ==' ');++i);

                if(i==size)
                {
                    return ParseResult::needMoreData;
                }
                else
                {
                    cur = i;
                    state = State::headerValue;
                }
            }
            else if(result == ReadResult::size)
            {
                return ParseResult::needMoreData;
            }
            else
            {
                return ParseResult::finishInvalid;
            }
        }
        else if(state == State::headerValue)
        {
            result = readData(CharClass::endOfLine, CharClass::notEndOfLine, length);
            if(result == CharClass::endOfLine)
            {
                Header h;
                h.key = curKey;
                h.value.start = cur;
                h.value.length = length;
                headers.push_back(h);
            }
        }

    }

    int cur = 0;
    char *data;
    int size;
    State state;

    int methodStart;
    int methodLength;

    int urlStart;
    int urlLength;

    int urlParametersStart;
    int urlParametersLength;

    struct Field
    {
        int start;
        int length;
    };

    struct Header
    {
        Field key;
        Field value;
    };

    Field curKey;

    std::vector<Header> headers;
};


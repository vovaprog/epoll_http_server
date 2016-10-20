#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <time.h>
#include <vector>

class HttpRequest
{
public:
    ~HttpRequest()
    {
        if(urlBuffer != nullptr)
        {
            delete[] urlBuffer;
            urlBuffer = nullptr;
        }
    }


    enum class ParseResult
    {
        finishOk, finishInvalid, needMoreData
    };


    ParseResult startParse(const char *data, int size);

    ParseResult continueParse(const char *data, int size);

    const char* getUrl();

    int getHeaderValue(const char *key, const char **ptr, int *size);

    time_t getIfModifiedSince();

    int print();


protected:

    enum class State
    {
        method, spaceAfterMethod, url, urlParameters, spaceAfterUrl, headerKey, headerSpace, headerValue, headerLineBreak, finishOk
    };
    enum class ReadResult
    {
        invalid, needMoreData, ok, question, endOfHeaders
    };

    ReadResult readMethod(int &length);
    ReadResult readSpaces(int &length);
    ReadResult readUrl(int &length);
    ReadResult readUrlParameters(int &length);
    ReadResult readToNextLine(int &length);
    ReadResult readHeaderKey(int &length);
    ReadResult readHeaderSpace(int &length);
    ReadResult readHeaderValue(int &length);
    ReadResult readHeaderLineBreak(int &length);

    ParseResult parse();

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

        urlDecoded = false;
    }


protected:

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

    char *urlBuffer = nullptr;
    int urlBufferSize = 0;
    bool urlDecoded = false;
};

#endif // HTTP_REQUEST_H


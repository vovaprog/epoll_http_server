#include <HttpRequest.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <chrono>

void check_true(bool x, const char *file, int line, const char *function, const char *expression)
{
    if(x)
    {
        printf("%s:%d   function: %s   ( %s ) : ok\n", file, line, function, expression);
    }
    else
    {
        printf("%s:%d   function: %s   ( %s ) : error!\n", file, line, function, expression);
        exit(-1);
    }
}


#define CHECK_TRUE(x) check_true((x), __FILE__, __LINE__, __func__, #x)


void test1()
{
    const char *data =
        "GET /gallery/album/flowers%202016/1?album-view=3 HTTP/1.1\r\n"
        "Host: 127.0.0.1:7000\r\n"
        "User-Agent: Mozilla Firefox\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml\r\n"
        "Accept-Language: en-US,en\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "If-Modified-Since: Mon, 26 Sep 2016 10:15:30 GMT\r\n"
        "Cache-Control: max-age=0\r\n\r\n";

    int dataLen = strlen(data);

    printf("sizeof(HttpRequest): %d\n", (int)sizeof(HttpRequest));

    HttpRequest request;

    HttpRequest::ParseResult result = request.parse(data, 50);

    CHECK_TRUE(result == HttpRequest::ParseResult::needMoreData);

    for(int i = 50; i <= dataLen - 1 && result != HttpRequest::ParseResult::finishOk; i += 50)
    {
        result = request.parse(data, i);
        CHECK_TRUE(result == HttpRequest::ParseResult::needMoreData);
    }

    result = request.parse(data, dataLen);
    CHECK_TRUE(result == HttpRequest::ParseResult::finishOk);


    request.reset();
    result = request.parse(data, dataLen);

    CHECK_TRUE(result == HttpRequest::ParseResult::finishOk);

    printf("\n");
    request.print();
    printf("\n");

    const char *url = request.getUrl();
    CHECK_TRUE(url != nullptr);
    printf("getUrl: [%s]\n", url);


    const char *ptr;
    int length;


    CHECK_TRUE(request.getHeaderValue("if-modified-since", &ptr, &length) == 0);
    printf("if-modified-since: [%.*s]\n", length, ptr);


    CHECK_TRUE(request.getHeaderValue("Accept-Encoding", &ptr, &length) == 0);
    printf("Accept-Encoding: [%.*s]\n", length, ptr);


    time_t modSince = request.getIfModifiedSince();

    CHECK_TRUE(modSince != 0);
    printf("If-Modified-Since time: %lld\n", (long long int)modSince);


    CHECK_TRUE(request.isUrlPrefix("/gallery"));
    CHECK_TRUE(request.isUrlPrefix("/gallery/"));
    CHECK_TRUE(request.isUrlPrefix("/gallery/album"));
    CHECK_TRUE(request.isUrlPrefix("/gallery/album/"));
    CHECK_TRUE(request.isUrlPrefix("/gal") == false);
    CHECK_TRUE(request.isUrlPrefix("/galleryy") == false);
    CHECK_TRUE(request.isUrlPrefix("/gallery/al") == false);
    CHECK_TRUE(request.isUrlPrefix("/gallery/albu") == false);
}


void test2()
{
    const char *dataPost = "POST /calendar/year/month/day HTTP/1.1\r\n"
                           "Host: 127.0.0.1:7000\r\n"
                           "User-Agent: Mozilla Firefox\r\n"
                           "Accept: text/html,application/xhtml+xml,application/xml\r\n"
                           "Accept-Language: en-US,en\r\n"
                           "Accept-Encoding: gzip, deflate, br\r\n"
                           "Connection: keep-alive\r\n"
                           "Content-Type: application/x-www-form-urlencoded\r\n"
                           "Content-Length: 50\r\n\r\n"
                           "12345678901234567890123456789012345678901234567890";

    size_t dataLen = strlen(dataPost);

    HttpRequest request;
    HttpRequest::ParseResult result;

    result = request.parse(dataPost, 50);
    CHECK_TRUE(result == HttpRequest::ParseResult::needMoreData);

    result = request.parse(dataPost, dataLen - 1);
    CHECK_TRUE(result == HttpRequest::ParseResult::needMoreData);

    result = request.parse(dataPost, dataLen);
    CHECK_TRUE(result == HttpRequest::ParseResult::finishOk);

    printf("\n");
    request.print();
    printf("\n");

    const char *url = request.getUrl();
    CHECK_TRUE(url != nullptr);

    CHECK_TRUE(strcmp(url, "/calendar/year/month/day") == 0);
}

long long int getMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()
           ).count();
}

void testPerformance()
{
    const char *data =
        "GET /gallery/album/flowers%202016/1?album-view=3 HTTP/1.1\r\n"
        "Host: 127.0.0.1:7000\r\n"
        "User-Agent: Mozilla Firefox\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml\r\n"
        "Accept-Language: en-US,en\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Connection: keep-alive\r\n"
        "If-Modified-Since: Mon, 26 Sep 2016 10:15:30 GMT\r\n"
        "Cache-Control: max-age=0\r\n\r\n";

    int dataLen = strlen(data);

    HttpRequest request;

    const int iterCount = 10000000;

    unsigned long long int millisStart = getMilliseconds();

    for(int i = 0; i < iterCount; ++i)
    {
        if(request.parse(data, dataLen) != HttpRequest::ParseResult::finishOk)
        {
            printf("parse failed!\n");
            exit(-1);
        }
    }

    unsigned long long int millisDif = getMilliseconds() - millisStart;

    printf("performance test milliseconds: %llu\n", millisDif);
}

int main()
{
    test1();
    test2();
    testPerformance();

    printf("\n============\nall tests ok\n");

    return 0;
}

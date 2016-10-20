    #include <stdio.h>
    #include <string.h>

    #include <HttpRequest.h>

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

        HttpRequest request;

        int startLen = 50;
        HttpRequest::ParseResult result = request.startParse(data, 50);

        for(int i=startLen + 1;i<=dataLen && result != HttpRequest::ParseResult::finishOk;++i)
        {
            result = request.continueParse(data, i);
        }

        if(result == HttpRequest::ParseResult::finishOk)
        {
            request.print();

            const char *url = request.getUrl();
            if(url)
            {
                printf("getUrl: [%s]\n", url);
            }
            else
            {
                printf("getUrl: null\n");
            }

            const char *ptr;
            int length;
            if(request.getHeaderValue("if-modified-since", &ptr, &length) == 0)
            {
                printf("if-modified-since: [%.*s]\n", length, ptr);
            }
            else
            {
                printf("if-modified-since: not found\n");
            }

            if(request.getHeaderValue("Accept-Encoding", &ptr, &length) == 0)
            {
                printf("Accept-Encoding: [%.*s]\n", length, ptr);
            }
            else
            {
                printf("Accept-Encoding: not found\n");
            }

            time_t modSince = request.getIfModifiedSince();

            printf("If-Modified-Since time: %lld\n", (long long int)modSince);
        }
    }


    int main()
    {
        test1();

        return 0;
    }

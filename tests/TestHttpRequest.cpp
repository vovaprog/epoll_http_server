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

        printf("sizeof(HttpRequest): %d\n", (int)sizeof(HttpRequest));

        HttpRequest request;

        int startLen = 50;
        HttpRequest::ParseResult result = request.parse(data, 50);

        for(int i=startLen + 1;i<=dataLen && result != HttpRequest::ParseResult::finishOk;++i)
        {
            result = request.parse(data, i);
        }

        request.reset();
        result = request.parse(data, dataLen);

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

            if(request.isUrlPrefix("/gallery")) printf("request.isUrlPrefix(/gallery)   ok\n");
            else printf("request.isUrlPrefix(/gallery)   no\n");

            if(request.isUrlPrefix("/gal")) printf("request.isUrlPrefix(/gal)   ok\n");
            else printf("request.isUrlPrefix(/gal)   no\n");

            if(request.isUrlPrefix("gallery")) printf("request.isUrlPrefix(gallery)   ok\n");
            else printf("request.isUrlPrefix(gallery)   no\n");

            if(request.isUrlPrefix("gal")) printf("request.isUrlPrefix(gal)   ok\n");
            else printf("request.isUrlPrefix(gal)   no\n");

            if(request.isUrlPrefix("galleryy")) printf("request.isUrlPrefix(galleryy)   ok\n");
            else printf("request.isUrlPrefix(galleryy)   no\n");

            if(request.isUrlPrefix("/gallery/")) printf("request.isUrlPrefix(/gallery/)   ok\n");
            else printf("request.isUrlPrefix(/gallery/)   no\n");

            if(request.isUrlPrefix("/gallery//")) printf("request.isUrlPrefix(/gallery//)   ok\n");
            else printf("request.isUrlPrefix(/gallery//)   no\n");

            if(request.isUrlPrefix("/gallery/al")) printf("request.isUrlPrefix(/gallery/al)   ok\n");
            else printf("request.isUrlPrefix(/gallery/al)   no\n");

            if(request.isUrlPrefix("/gallery/album")) printf("request.isUrlPrefix(/gallery/album)   ok\n");
            else printf("request.isUrlPrefix(/gallery/album)   no\n");

            if(request.isUrlPrefix("/gallery/album///")) printf("request.isUrlPrefix(/gallery/album///)   ok\n");
            else printf("request.isUrlPrefix(/gallery/album///)   no\n");

            if(request.isUrlPrefix("/gallery/albu")) printf("request.isUrlPrefix(/gallery/albu)   ok\n");
            else printf("request.isUrlPrefix(/gallery/albu)   no\n");
        }


        const char *dataPost = "POST /calendar/save-task HTTP/1.1\r\n"
        "Host: 127.0.0.1:7000\r\n"
        "User-Agent: Mozilla Firefox\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml\r\n"
        "Accept-Language: en-US,en\r\n"
        "Accept-Encoding: gzip, deflate, br\r\n"
        "Connection: keep-alive\r\n"
        "Content-Type: application/x-www-form-urlencoded\r\n"
        "Content-Length: 50\r\n\r\n"
        "12345678901234567890123456789012345678901234567890";

        dataLen = strlen(dataPost);

        request.reset();
        result = request.parse(dataPost, dataLen);

        if(result == HttpRequest::ParseResult::finishOk)
        {
            request.print();
        }
    }


    int main()
    {
        test1();

        return 0;
    }

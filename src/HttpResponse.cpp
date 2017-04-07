#include <HttpResponse.h>
#include <TimeUtils.h>

#include <time.h>
#include <stdio.h>
#include <string.h>

int HttpResponse::ok200(char *buffer, int size, long long int contentLength, time_t lastModified)
{
    char lastModifiedString[80];
    strftime(lastModifiedString, sizeof(lastModifiedString), RFC1123FMT, gmtime(&lastModified));

    int ret = snprintf(buffer, size,
                       "HTTP/1.1 200 Ok\r\n"
                       "Content-Length: %lld\r\n"
                       "Last-Modified: %s\r\n"
                       "Connection: close\r\n\r\n", contentLength, lastModifiedString);

    if(ret >= size)
    {
        return -1;
    }

    return ret;
}


int HttpResponse::notFound404(char *buffer, int size)
{
    const char *html =
        "<html><head>"
        "<title>404 Not Found</title>"
        "</head><body>"
        "<h1>Not Found</h1>"
        "<p>The requested URL was not found on this server.</p>"
        "</body></html>";

    size_t htmlLength = strlen(html);

    int ret = snprintf(buffer, size,
                       "HTTP/1.1 404 Not Found\r\n"
                       "Content-Length: %zu\r\n"
                       "Connection: close\r\n\r\n%s", htmlLength, html);

    if(ret >= size)
    {
        return -1;
    }

    return ret;
}


int HttpResponse::notModified304(char *buffer, int size)
{
    int ret = snprintf(buffer, size,
                       "HTTP/1.1 304 Not Modified\r\n"
                       "Connection: close\r\n\r\n");

    if(ret >= size)
    {
        return -1;
    }

    return ret;
}


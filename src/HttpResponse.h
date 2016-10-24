#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <time.h>

enum HttpCode { ok = 200, notModified = 304, notFound = 404 };

class HttpResponse {
public:

    static int ok200(char *buffer, int size, long long int contentLength, time_t lastModified);
    static int notFound404(char *buffer, int size);
    static int notModified304(char *buffer, int size);

};

#endif


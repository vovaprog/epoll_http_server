#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

enum HttpCode { ok = 200, notModified = 304, notFound = 404 };


void initHttpUtils();

const char* httpCode2String(int code);

#endif

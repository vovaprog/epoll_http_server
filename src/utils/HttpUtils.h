#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H

enum HttpCode { ok = 200, notFound = 404 };


void initHttpUtils();

const char* httpCode2String(int code);

int percentDecode(const char *src, char *dst, int srcLength);

#endif

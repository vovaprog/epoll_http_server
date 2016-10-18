#ifndef TIME_UTILS_H
#define TIME_UTILS_H

long long int getMilliseconds();

int getCurrentTimeString(char *timeBuffer, int timeBufferSize);

#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

#endif


#ifndef SSL_UTILS_H
#define SSL_UTILS_H

#include <ExecutorData.h>

class Executor;
class Log;

ssize_t sslWriteFd0(Executor *exec, ExecutorData &data, const void *buf, size_t count, int &errorCode, Log *log);

#endif



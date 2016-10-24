#include <SslUwsgiExecutor.h>
#include <PollLoopBase.h>
#include <SslUtils.h>

#include <errno.h>
#include <sys/epoll.h>
#include <openssl/ssl.h>

ssize_t SslUwsgiExecutor::writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode)
{
    return sslWriteFd0(this, data, buf, count, errorCode, log);
}


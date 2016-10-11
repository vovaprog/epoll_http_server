#include <SslUwsgiExecutor.h>

#include <openssl/ssl.h>

ssize_t SslUwsgiExecutor::writeFd0(ExecutorData &data, const void *buf, size_t count)
{
    return SSL_write(data.ssl, buf, count);
}


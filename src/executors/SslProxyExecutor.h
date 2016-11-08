#ifndef SSL_PROXY_EXECUTOR_H
#define SSL_PROXY_EXECUTOR_H

#include <ProxyExecutorReadWrite.h>

class SslProxyExecutor: public ProxyExecutorReadWrite
{
public:

    const char* name() const override
    {
        return "sslproxy";
    }

protected:

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode) override;
};

#endif


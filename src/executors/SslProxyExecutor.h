#ifndef SSL_PROXY_EXECUTOR_H
#define SSL_PROXY_EXECUTOR_H

#include <ProxyExecutor.h>

class SslProxyExecutor: public ProxyExecutor
{
public:

    const char* name() override
    {
        return "sslproxy";
    }

protected:

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode) override;
};

#endif


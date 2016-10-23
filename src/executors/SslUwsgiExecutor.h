#ifndef SSL_UWSGI_EXECUTOR_H
#define SSL_UWSGI_EXECUTOR_H

#include <UwsgiExecutor.h>

class SslUwsgiExecutor: public UwsgiExecutor
{
public:

    const char* name() override
    {
        return "ssluwsgi";
    }

protected:

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode) override;
};

#endif


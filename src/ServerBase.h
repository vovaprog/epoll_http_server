#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#include <Log.h>
#include <ExecutorType.h>

#ifdef USE_SSL
#    include <openssl/ssl.h>
#endif


class ServerBase
{
public:

    virtual int createRequestExecutor(int fd, ExecutorType execType) = 0;

    Log *log = nullptr;

#ifdef USE_SSL
    SSL_CTX* sslCtx = nullptr;
#endif
};

#endif


#ifndef SERVER_BASE_H
#define SERVER_BASE_H

#include <Log.h>
#include <ExecutorType.h>

#include <openssl/ssl.h>

class ServerBase
{
public:

    virtual int createRequestExecutor(int fd, ExecutorType execType) = 0;

    Log *log = nullptr;
    SSL_CTX* sslCtx = nullptr;
};

#endif


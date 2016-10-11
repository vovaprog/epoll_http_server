#ifndef SERVER_H
#define SERVER_H

#include <ServerParameters.h>
#include <ExecutorType.h>
#include <PollLoop.h>

#include <thread>


class Server: public ServerBase
{
public:

    ~Server()
    {
        stop();
    }

    int start(ServerParameters &parameters);

    void stop();

    int createRequestExecutor(int fd, ExecutorType execType) override;

    void logStats();

    static void cleanup();

protected:

    static int sslInit();

    SSL_CTX* sslCreateContext(Log *log);

    void sslDestroyContext(SSL_CTX *ctx);


protected:

    static bool sslInited;

    ServerParameters parameters;

    PollLoop *loops = nullptr;
    std::thread *threads = nullptr;
};

#endif


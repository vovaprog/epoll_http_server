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

    void logStats() const;

    static int  staticInit();
    static void staticDestroy();

protected:

    void threadEntry(int pollLoopIndex);

#ifdef USE_SSL
    SSL_CTX* sslCreateContext(Log *log);

    void sslDestroyContext(SSL_CTX *ctx);

    static int sslInit();

protected:

    static bool sslInited;
#endif

protected:

    ServerParameters parameters;

    PollLoop *loops = nullptr;
    std::thread *threads = nullptr;
};

#endif


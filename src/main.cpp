#include <Server.h>

#include <signal.h>
#include <atomic>
#include <pthread.h>


std::atomic_bool runFlag;


static void sigIntHandler(int /*signalNumber*/)
{
    printf("sig int\n");

    runFlag.store(false);
}


static int installSigIntHandler()
{
    runFlag.store(true);

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sigIntHandler;
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) != 0)
    {
        perror("sigaction failed");
        return -1;
    }

    return 0;
}


int main(int argc, char** argv)
{
    const char *fileName = "./config.xml";

    printf("USAGE: epoll_http_server config_file_name (default: %s)\n", fileName);

    if(argc > 1)
    {
        fileName = argv[1];
    }

    if(installSigIntHandler() != 0)
    {
        return -1;
    }

    Server::staticInit();

    Server srv;

    ServerParameters params;
    if(params.load(fileName) != 0)
    {
        return -1;
    }

    if(srv.start(params) != 0)
    {
        return -1;
    }

    pthread_setname_np(pthread_self(), "main");

    while(runFlag.load())
    {
        srv.logStats();
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }

    srv.stop();

    Server::staticDestroy();

    return 0;
}

#include <Server.h>

#include <signal.h>
#include <atomic>


std::atomic_bool runFlag;

static void sig_int_handler(int i)
{
    printf("sig int\n");

    runFlag.store(false);
}


int main(int argc, char** argv)
{
    const char *fileName = "./config.xml";

    printf("USAGE: http_server config_file_name (default: %s)\n", fileName);

    if(argc > 1)
    {
        fileName = argv[1];
    }

    runFlag.store(true);
    signal(SIGINT, sig_int_handler);

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

    while(runFlag.load())
    {
        srv.logStats();
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }

    srv.stop();

    Server::staticDestroy();

    return 0;
}

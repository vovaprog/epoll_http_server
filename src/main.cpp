#include <Server.h>

#include <HttpRequest.h>

#include <signal.h>
#include <atomic>


std::atomic_bool runFlag;

static void sig_int_handler(int i)
{
    printf("sig int\n");

    runFlag.store(false);
}

void testHttpRequest()
{
    const char *data = "GET /gallery/album/flowers/1?album-view=3 HTTP/1.1\r\n"
    "Host: 127.0.0.1:7000\r\n\r\n";

    HttpRequest request;

    HttpRequest::ParseResult result = request.startParse(data, strlen(data));

    if(result == HttpRequest::ParseResult::finishOk)
    {
        request.print();
    }

}


int main(int argc, char** argv)
{
    testHttpRequest();

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

#include <ExecutorData.h>

#include <unistd.h>
#include <openssl/ssl.h>

void ExecutorData::down()
{
    if(ssl != nullptr)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if(fd0 > 0)
    {
        close(fd0);
        fd0 = -1;
    }
    if(fd1 > 0)
    {
        close(fd1);
        fd1 = -1;
    }
    pollIndexFd0 = -1;
    pollIndexFd1 = -1;

    state = State::invalid;
    pExecutor = nullptr;

    bytesToSend = 0;
    filePosition = 0;

    buffer.clear();

    removeOnTimeout = true;

    return;
}


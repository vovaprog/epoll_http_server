#include <ExecutorData.h>
#include <Log.h>
#include <Executor.h>

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
    pollData0 = nullptr;
    pollData1 = nullptr;

    if(pipeReadFd > 0)
    {
        close(pipeReadFd);
        pipeReadFd = -1;
    }
    if(pipeWriteFd > 0)
    {
        close(pipeWriteFd);
        pipeWriteFd = -1;
    }
    bytesInPipe = 0;

    state = State::invalid;
    pExecutor = nullptr;

    bytesToSend = 0;
    filePosition = 0;

    buffer.clear();

    createTime = 0;
    lastProcessTime = 0;
    removeOnTimeout = true;

    connectionType = (int)ConnectionType::none;

    retryCounter = 0;

    proxy = nullptr;

    return;
}


void ExecutorData::writeLog(Log *log, Log::Level level, const char *title) const
{
    const char *execString;
    if(pExecutor != nullptr)
    {
        execString = pExecutor->name();
    }
    else
    {
        execString = "null";
    }

    log->writeLog(level, "%s   executor: %s   state: %d   fd0: %d   fd1: %d\n", title,  execString, (int)state, fd0, fd1);
}

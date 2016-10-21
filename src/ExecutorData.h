#ifndef EXECUTOR_DATA_H
#define EXECUTOR_DATA_H

#include <TransferRingBuffer.h>
#include <ConnectionType.h>
#include <Log.h>
#include <HttpRequest.h>

#include <sys/types.h>
#include <openssl/ssl.h>

class Executor;

struct ExecutorData
{
    ~ExecutorData()
    {
        down();
    }

    void down();

    void writeLog(Log *log, Log::Level level, const char *title);


    static const int REQUEST_BUFFER_SIZE = 10000;

    enum class State
    {
        invalid, readRequest, sendResponse, sendFile,
        forwardRequest, forwardResponse, forwardResponseOnlyWrite,
        sslHandshake, waitConnect, sendError, ok
    };

    int index = -1;

    Executor *pExecutor = nullptr;

    State state = State::invalid;

    int fd0 = -1;
    int fd1 = -1;
    int pollIndexFd0 = -1;
    int pollIndexFd1 = -1;

    long long int bytesToSend = 0;
    off_t filePosition = 0;

    TransferRingBuffer buffer;

    int port = 0;

    SSL *ssl = nullptr;

    static const long long int MAX_TIME_TO_LIVE_MILLIS = 1000 * 60 * 30; // 30 minutes

    long long int createTime = 0;
    long long int lastProcessTime = 0;
    bool removeOnTimeout = true;

    int connectionType = (int)ConnectionType::none;

    static const int MAX_RETRY_COUNTER = 1000;
    int retryCounter = 0;

    HttpRequest request;
};

#endif

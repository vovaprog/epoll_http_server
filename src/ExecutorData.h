#ifndef EXECUTOR_DATA_H
#define EXECUTOR_DATA_H

#include <TransferRingBuffer.h>
#include <ConnectionType.h>
#include <Log.h>
#include <HttpRequest.h>
#include <BlockStorage.h>

#include <sys/types.h>

#ifdef USE_SSL
#    include <openssl/ssl.h>
#endif

class Executor;
struct ProxyParameters;
struct PollData;

struct ExecutorData
{
    ExecutorData() = default;

    ExecutorData(const ExecutorData &ed) = delete;
    ExecutorData(ExecutorData &&ed) = delete;
    ExecutorData& operator=(const ExecutorData &ed) = delete;
    ExecutorData& operator=(ExecutorData && ed) = delete;

    ~ExecutorData()
    {
        down();
    }

    void down();

    void writeLog(Log *log, Log::Level level, const char *title) const;


    static const int REQUEST_BUFFER_SIZE = 10000;

    enum class State
    {
        invalid, readRequest, sendHeaders, sendFile,
        forwardRequest, forwardResponse, forwardResponseOnlyWrite,
        waitConnect, sendOnlyHeaders, ok,

#ifdef USE_SSL
        sslHandshake
#endif
    };

    Executor *pExecutor = nullptr;

    State state = State::invalid;

    int fd0 = -1;
    int fd1 = -1;
    PollData *pollData0 = nullptr;
    PollData *pollData1 = nullptr;

    int pipeReadFd = -1;
    int pipeWriteFd = -1;
    int bytesInPipe = 0;

    long long int bytesToSend = 0;
    off_t filePosition = 0;

    TransferRingBuffer buffer;

    int port = 0;

#ifdef USE_SSL
    SSL *ssl = nullptr;
#endif

    static const long long int MAX_TIME_TO_LIVE_MILLIS = 1000 * 60 * 30; // 30 minutes

    long long int createTime = 0;
    long long int lastProcessTime = 0;
    bool removeOnTimeout = true;

    int connectionType = (int)ConnectionType::none;

    static const int MAX_RETRY_COUNTER = 1000;
    int retryCounter = 0;

    HttpRequest request;

    ProxyParameters *proxy = nullptr;

    BlockStorage<ExecutorData>::ServiceData blockStorageData;
};

#endif

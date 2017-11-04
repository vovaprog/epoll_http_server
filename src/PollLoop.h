#ifndef POLL_LOOP_H
#define POLL_LOOP_H

#include <PollLoopBase.h>

#include <NewFdExecutor.h>
#include <ServerExecutor.h>
#include <RequestExecutor.h>
#include <FileExecutor.h>
#include <ProxyExecutorReadWrite.h>
#include <ProxyExecutorSplice.h>

#ifdef USE_SSL
#    include <SslServerExecutor.h>
#    include <SslRequestExecutor.h>
#    include <SslFileExecutor.h>
#    include <SslProxyExecutor.h>
#endif

#include <PollData.h>
#include <BlockStorage.h>

#include <sys/epoll.h>
#include <atomic>
#include <mutex>
#include <boost/lockfree/spsc_queue.hpp>


class PollLoop: public PollLoopBase
{
public:
    PollLoop() = default;

    ~PollLoop()
    {
        destroy();
    }

    int init(ServerBase *srv, ServerParameters *params);

    int run();

    void stop();

    int enqueueClientFd(int fd, ExecutorType execType);

    int checkNewFd() override;

    int createRequestExecutor(int fd, ExecutorType execType) override;

    int addPollFd(ExecutorData &data, int fd, int events) override;

    int editPollFd(ExecutorData &data, int fd, int events) override;

    int removePollFd(ExecutorData &data, int fd) override;

    int closeFd(ExecutorData &data, int fd) override;

    int listenPort(int port, ExecutorType execType);

    int numberOfPollFds() const;

protected:

    void checkTimeout(long long int curMillis);

    int createEventFd();

    void destroy();

    int initDataStructs(const ServerParameters *params);

    Executor* getExecutor(ExecutorType execType) override;

    ExecutorData* createExecutorData() override;

    void removeExecutorData(ExecutorData *execData) override;

    int createRequestExecutorInternal(int fd, ExecutorType execType);

    void logStats();


protected:

    NewFdExecutor newFdExecutor;
    ServerExecutor serverExecutor;
    RequestExecutor requestExecutor;
    FileExecutor fileExecutor;
    ProxyExecutorSplice proxyExecutor;

#ifdef USE_SSL
    SslServerExecutor sslServerExecutor;
    SslRequestExecutor sslRequestExecutor;
    SslFileExecutor sslFileExecutor;
    SslProxyExecutor sslProxyExecutor;
#endif

    BlockStorage<ExecutorData> execDatas;
    BlockStorage<PollData> pollDatas;


    static const int MAX_EPOLL_EVENTS = 1000;
    epoll_event events[MAX_EPOLL_EVENTS];

    std::vector<ExecutorData*> removeExecDatas;


    struct NewFdData
    {
        ExecutorType execType;
        int fd;
    };

    boost::lockfree::spsc_queue<NewFdData, boost::lockfree::capacity<1000>> newFdsQueue;
    std::mutex newFdsMutex;


    int epollFd = -1;
    int eventFd = -1;

    long long int lastCheckTimeoutMillis = 0;


    std::atomic_bool runFlag;
    std::atomic_int numOfPollFds;
};

#endif



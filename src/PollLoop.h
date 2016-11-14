#ifndef POLL_LOOP_H
#define POLL_LOOP_H

#include <PollLoopBase.h>

#include <NewFdExecutor.h>
#include <ServerExecutor.h>
#include <RequestExecutor.h>
#include <FileExecutor.h>
#include <ProxyExecutorReadWrite.h>
#include <SslServerExecutor.h>
#include <SslRequestExecutor.h>
#include <SslFileExecutor.h>
#include <SslProxyExecutor.h>
#include <ProxyExecutorSplice.h>

#include <PollData.h>

#include <sys/epoll.h>
#include <atomic>
#include <stack>
#include <set>
#include <mutex>
#include <boost/lockfree/spsc_queue.hpp>


class PollLoop: public PollLoopBase
{
public:
    PollLoop(): newFdsQueue(MAX_NEW_FDS) { }

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

    int numberOfPollFds();

    int addPollFd(ExecutorData &data, int fd, int events) override;

    int editPollFd(ExecutorData &data, int fd, int events) override;

    int removePollFd(ExecutorData &data, int fd) override;

    int closeFd(ExecutorData &data, int fd) override;

    int listenPort(int port, ExecutorType execType);

protected:

    void checkTimeout(long long int curMillis);

    int createEventFd();

    void destroy();

    int initDataStructs();

    Executor* getExecutor(ExecutorType execType) override;

    ExecutorData* createExecutorData() override;

    void removeExecutorData(ExecutorData *execData) override;

    int createRequestExecutorInternal(int fd, ExecutorType execType);

    ExecutorData* getExecData(int index);
    PollData* getPollData(int index);

    void logStats();


protected:

    NewFdExecutor newFdExecutor;
    ServerExecutor serverExecutor;
    SslServerExecutor sslServerExecutor;
    RequestExecutor requestExecutor;
    SslRequestExecutor sslRequestExecutor;
    FileExecutor fileExecutor;
    SslFileExecutor sslFileExecutor;
    ProxyExecutorSplice proxyExecutor;
    SslProxyExecutor sslProxyExecutor;


    ExecutorData **execDatas = nullptr;
    PollData **pollDatas = nullptr;

    static const int MAX_EPOLL_EVENTS = 1000;
    epoll_event events[MAX_EPOLL_EVENTS];

    std::stack<int, std::vector<int>> emptyExecDatas;
    std::stack<int, std::vector<int>> emptyPollDatas;

    std::set<int> usedExecDatas;
    std::vector<ExecutorData*> removeExecDatas;

    std::atomic_int numOfPollFds;


    int execDataBlocks = 0, execDataBlockSize = 0;
    int pollDataBlocks = 0, pollDataBlockSize = 0;

    std::atomic_bool runFlag;


    static const int MAX_NEW_FDS = 1000;

    struct NewFdData
    {
        ExecutorType execType;
        int fd;
    };

    boost::lockfree::spsc_queue<NewFdData> newFdsQueue;
    std::mutex newFdsMutex;


    int epollFd = -1;
    int eventFd = -1;

    long long int lastCheckTimeoutMillis = 0;
};

#endif

#ifndef POLL_LOOP_H
#define POLL_LOOP_H

#include <PollLoopBase.h>

#include <NewFdExecutor.h>
#include <ServerExecutor.h>
#include <RequestExecutor2.h>
#include <FileExecutor.h>
#include <UwsgiExecutor.h>
#include <SslServerExecutor.h>
#include <SslRequestExecutor.h>
#include <SslFileExecutor.h>
#include <SslUwsgiExecutor.h>

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

    void logStats();


protected:

    ServerExecutor serverExecutor;
    SslServerExecutor sslServerExecutor;
    SslFileExecutor sslFileExecutor;
    SslUwsgiExecutor sslUwsgiExecutor;
    RequestExecutor2 requestExecutor;
    SslRequestExecutor sslRequestExecutor;
    FileExecutor fileExecutor;
    UwsgiExecutor uwsgiExecutor;
    NewFdExecutor newFdExecutor;


    ExecutorData *execDatas = nullptr;
    PollData *pollDatas = nullptr;
    epoll_event *events = nullptr;

    std::stack<int, std::vector<int>> emptyExecDatas;
    std::stack<int, std::vector<int>> emptyPollDatas;

    std::set<int> usedExecDatas;
    std::vector<int> removeExecDatas;

    std::atomic_int numOfPollFds;


    int MAX_EXECUTORS = 0, MAX_EVENTS = 0;

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

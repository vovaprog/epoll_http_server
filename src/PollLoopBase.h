#ifndef POLL_LOOP_BASE_H
#define POLL_LOOP_BASE_H

#include <Log.h>
#include <ServerBase.h>
#include <ExecutorType.h>
#include <ExecutorData.h>
#include <ServerParameters.h>


class PollLoopBase
{
public:
    virtual ExecutorData* createExecutorData() = 0;
    virtual void removeExecutorData(ExecutorData* data) = 0;

    virtual Executor* getExecutor(ExecutorType execType) = 0;

    virtual int addPollFd(ExecutorData &data, int fd, int events) = 0;
    virtual int editPollFd(ExecutorData &data, int fd, int events) = 0;
    virtual int removePollFd(ExecutorData &data, int fd) = 0;

    virtual int createRequestExecutor(int fd, ExecutorType execType) = 0;
    virtual int checkNewFd() = 0;


    Log *log = nullptr;

    static const int MAX_FILE_NAME = 300;
    char fileNameBuffer[MAX_FILE_NAME + 1];
    int rootFolderLength = 0;

    ServerParameters *parameters = nullptr;

    ServerBase *srv = nullptr;
};

#endif

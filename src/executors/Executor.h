#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <ProcessResult.h>
#include <ExecutorData.h>
#include <Log.h>

class PollLoopBase;

class Executor
{
public:

    virtual int init(PollLoopBase *loop) = 0;

    virtual int up(ExecutorData &data) = 0;

    virtual ProcessResult process(ExecutorData &data, int fd, int events) = 0;

    virtual const char *name() = 0;

protected:

    virtual ssize_t readFd0(ExecutorData &data, void *buf, size_t count, int &errorCode);
    virtual ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode);

public:

    PollLoopBase *loop = nullptr;
    Log *log = nullptr;
};

#endif

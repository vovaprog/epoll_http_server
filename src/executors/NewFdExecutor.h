#ifndef NEW_FD_EXECUTOR_H
#define NEW_FD_EXECUTOR_H

#include <Executor.h>

class NewFdExecutor: public Executor
{
public:
    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;
};

#endif


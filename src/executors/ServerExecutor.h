#ifndef SERVER_EXECUTOR_H
#define SERVER_EXECUTOR_H

#include <Executor.h>

class ServerExecutor: public Executor
{
public:
    int init(PollLoopBase *srv) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() const override
    {
        return "server";
    }
};

#endif

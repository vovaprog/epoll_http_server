#ifndef SSL_SERVER_EXECUTOR_H
#define SSL_SERVER_EXECUTOR_H

#include <Executor.h>

class SslServerExecutor: public Executor
{
public:
    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;
};

#endif

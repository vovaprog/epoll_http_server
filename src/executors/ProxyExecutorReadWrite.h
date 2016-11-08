#ifndef PROXY_EXECUTOR_READ_WRITE_H
#define PROXY_EXECUTOR_READ_WRITE_H

#include <Executor.h>

class ProxyExecutorReadWrite: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() override
    {
        return "proxyrw";
    }

protected:

    ProcessResult process_forwardRequest(ExecutorData &data);

    ProcessResult process_forwardResponseRead(ExecutorData &data);

    ProcessResult process_forwardResponseWrite(ExecutorData &data);

    ProcessResult process_waitConnect(ExecutorData &data);
};

#endif

#ifndef PROXY_EXECUTOR_SPLICE_H
#define PROXY_EXECUTOR_SPLICE_H

#include <Executor.h>

class ProxyExecutorSplice: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() const override
    {
        return "proxysplice";
    }

protected:

    int openPipe(ExecutorData &data);

    ProcessResult process_forwardRequest(ExecutorData &data);

    ProcessResult process_forwardResponseRead(ExecutorData &data);

    ProcessResult process_forwardResponseWrite(ExecutorData &data);

    ProcessResult process_waitConnect(ExecutorData &data);
};

#endif

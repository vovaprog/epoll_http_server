#ifndef UWSGI_EXECUTOR_H
#define UWSGI_EXECUTOR_H

#include <Executor.h>

class UwsgiExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() override
    {
        return "uwsgi";
    }

protected:

    ProcessResult process_forwardRequest(ExecutorData &data);

    ProcessResult process_forwardResponseRead(ExecutorData &data);

    ProcessResult process_forwardResponseWrite(ExecutorData &data);

    ProcessResult process_forwardResponseOnlyWrite(ExecutorData &data);

    ProcessResult process_waitConnect(ExecutorData &data);
};

#endif

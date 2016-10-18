#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <Executor.h>

class FileExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() override { return "file"; }

protected:

    int createResponse(ExecutorData &data);
    int createError(ExecutorData &data, int statusCode);

    virtual ProcessResult process_sendResponseSendData(ExecutorData &data);

    virtual ProcessResult process_sendFile(ExecutorData &data);
};

#endif
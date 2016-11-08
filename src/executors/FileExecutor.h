#ifndef FILE_EXECUTOR_H
#define FILE_EXECUTOR_H

#include <Executor.h>

#include <time.h>

class FileExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() const override
    {
        return "file";
    }

protected:

    int createOkResponse(ExecutorData &data, time_t lastModified);
    int createResponse(ExecutorData &data, int statusCode);

    virtual ProcessResult process_sendHeaders(ExecutorData &data);

    virtual ProcessResult process_sendFile(ExecutorData &data);
};

#endif

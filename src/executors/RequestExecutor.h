#ifndef REQUEST_EXECUTOR_H
#define REQUEST_EXECUTOR_H

#include <Executor.h>

class RequestExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() override { return "request"; }

protected:

    int readRequest(ExecutorData &data);

    int checkApplicationUrl(ExecutorData &data, char *urlBuffer);

    int checkFileUrl(ExecutorData &data, char *urlBuffer);

    bool isUrlPrefix(const char *url, const char *prefix);

    enum class ParseRequestResult
    {
        again, file, uwsgi, invalid
    };

    ParseRequestResult parseRequest(ExecutorData &data);

    ProcessResult setExecutor(ExecutorData &data, Executor *pExecutor);

    virtual ProcessResult process_readRequest(ExecutorData &data);
};

#endif

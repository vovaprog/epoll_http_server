#ifndef REQUEST_EXECUTOR2_H
#define REQUEST_EXECUTOR2_H

#include <Executor.h>

class RequestExecutor: public Executor
{
public:

    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() const override
    {
        return "request";
    }

protected:

    int readRequest(ExecutorData &data);

    int checkApplicationUrl(ExecutorData &data, char *urlBuffer);

    int checkFileUrl(ExecutorData &data, char *urlBuffer);

    bool isUrlPrefix(const char *url, const char *prefix);

    enum class ParseRequestResult
    {
        again, file, proxy, invalid
    };

    ParseRequestResult parseRequest(ExecutorData &data);

    ProcessResult setExecutor(ExecutorData &data, Executor *pExecutor);

    virtual ProcessResult process_readRequest(ExecutorData &data);

    ProxyParameters* findProxy(ExecutorData &data);
};

#endif

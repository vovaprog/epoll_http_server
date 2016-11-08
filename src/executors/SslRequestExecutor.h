#ifndef SSL_REQUEST_EXECUTOR_H
#define SSL_REQUEST_EXECUTOR_H

#include <RequestExecutor.h>

class SslRequestExecutor: public RequestExecutor
{
public:
    int init(PollLoopBase *loop) override;

    int up(ExecutorData &data) override;

    int sslInit(ExecutorData &data);

    enum class HandleHandshakeResult
    {
        ok, error, again
    };

    HandleHandshakeResult handleHandshake(ExecutorData &data, bool firstTime);

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() const override
    {
        return "sslrequest";
    }

protected:

    ssize_t readFd0(ExecutorData &data, void *buf, size_t count, int &errorCode) override;

    ProcessResult process_handshake(ExecutorData &data);

    ProcessResult process_readRequest(ExecutorData &data) override;
};

#endif


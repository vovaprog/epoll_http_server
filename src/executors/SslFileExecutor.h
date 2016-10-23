#ifndef SSL_FILE_EXECUTOR_H
#define SSL_FILE_EXECUTOR_H

#include <FileExecutor.h>

class SslFileExecutor: public FileExecutor
{
public:

    ProcessResult process(ExecutorData &data, int fd, int events) override;

    const char* name() override
    {
        return "sslfile";
    }

protected:

    ProcessResult process_sendFile(ExecutorData &data) override;

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode) override;

};

#endif


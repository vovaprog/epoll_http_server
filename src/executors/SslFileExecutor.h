#ifndef SSL_FILE_EXECUTOR_H
#define SSL_FILE_EXECUTOR_H

#include <FileExecutor.h>

class SslFileExecutor: public FileExecutor
{
protected:

    ssize_t writeFd0(ExecutorData &data, const void *buf, size_t count) override;

    ProcessResult process_sendFile(ExecutorData &data) override;
};

#endif


#ifndef SSL_FILE_EXECUTOR_H
#define SSL_FILE_EXECUTOR_H

#include <FileExecutor.h>

class SslFileExecutor: public FileExecutor
{
public:

    ProcessResult process(ExecutorData &data, int fd, int events) override;

protected:

    ProcessResult process_sendResponseSendData(ExecutorData &data) override;

    ProcessResult process_sendFile(ExecutorData &data) override;

};

#endif


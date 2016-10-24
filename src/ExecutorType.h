#ifndef EXECUTOR_TYPE_H
#define EXECUTOR_TYPE_H

enum class ExecutorType
{
    server, request,  file,  proxy,
    serverSsl, requestSsl, sslFile, sslProxy
};

#endif

#ifndef EXECUTOR_TYPE_H
#define EXECUTOR_TYPE_H

enum class ExecutorType
{
    server, request,  file,  proxy,

#ifdef USE_SSL
    serverSsl, requestSsl, sslFile, sslProxy
#endif
};

#endif

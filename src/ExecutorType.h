#ifndef EXECUTOR_TYPE_H
#define EXECUTOR_TYPE_H

enum class ExecutorType
{
    server, request,  file,  uwsgi,
    serverSsl, requestSsl, sslFile, sslUwsgi
};

#endif

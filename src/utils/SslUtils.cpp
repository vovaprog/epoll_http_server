#include <ExecutorData.h>
#include <PollLoopBase.h>
#include <Executor.h>
#include <Log.h>

#include <errno.h>
#include <openssl/ssl.h>
#include <sys/epoll.h>

ssize_t sslWriteFd0(Executor *exec, ExecutorData &data, const void *buf, size_t count, int &errorCode, Log *log)
{
    int result = SSL_write(data.ssl, buf, static_cast<int>(count));

    if(result > 0)
    {
        errorCode = 0;
    }
    else
    {
        int error = SSL_get_error(data.ssl, result);

        if(error == SSL_ERROR_WANT_WRITE)
        {
            errorCode = EAGAIN;
        }
        else if(error == SSL_ERROR_WANT_READ)
        {
            if(exec->loop->editPollFd(data, data.fd0, EPOLLIN | EPOLLOUT) == 0)
            {
                errorCode = EAGAIN;
            }
            else
            {
                errorCode = EINVAL;
            }
        }
        else
        {
            log->error("SSL_write failed. return: %d   error: %d   errno: %d   strerror: %s\n", result, error, errno, strerror(errno));
            errorCode = EINVAL;
        }
    }

    return result;
}


#include <SslUwsgiExecutor.h>
#include <PollLoopBase.h>

#include <errno.h>
#include <sys/epoll.h>
#include <openssl/ssl.h>

ssize_t SslUwsgiExecutor::writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode)
{
    int result = SSL_write(data.ssl, buf, count);

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
            if(loop->editPollFd(data, data.fd0, EPOLLIN | EPOLLOUT) == 0)
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
            errorCode = EINVAL;
        }
    }

    return result;
}


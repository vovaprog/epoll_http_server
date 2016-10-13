#include <Executor.h>

#include <unistd.h>
#include <errno.h>

ssize_t Executor::readFd0(ExecutorData &data, void *buf, size_t count, int &errorCode)
{
    int result = read(data.fd0, buf, count);

    if(result > 0)
    {
        errorCode = 0;
    }
    else
    {
        errorCode = errno;
    }

    return result;
}

ssize_t Executor::writeFd0(ExecutorData &data, const void *buf, size_t count, int &errorCode)
{
    int result = write(data.fd0, buf, count);

    if(result > 0)
    {
        errorCode = 0;
    }
    else
    {
        errorCode = errno;
    }

    return result;
}

#include <Executor.h>

#include <unistd.h>

ssize_t Executor::readFd0(ExecutorData &data, void *buf, size_t count)
{
    return read(data.fd0, buf, count);
}

ssize_t Executor::writeFd0(ExecutorData &data, const void *buf, size_t count)
{
    return write(data.fd0, buf, count);
}

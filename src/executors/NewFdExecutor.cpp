#include <NewFdExecutor.h>
#include <PollLoopBase.h>

#include <sys/eventfd.h>

int NewFdExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    log = loop->log;
    return 0;
}


int NewFdExecutor::up(ExecutorData &data)
{
    data.removeOnTimeout = false;

    return 0;
}

ProcessResult NewFdExecutor::process(ExecutorData &data, int fd, int events)
{
    eventfd_t val;
    if(eventfd_read(fd, &val) != 0)
    {
        log->error("eventfd_failed\n");
        return ProcessResult::shutdown;
    }

    loop->checkNewFd();
    return ProcessResult::ok;
}

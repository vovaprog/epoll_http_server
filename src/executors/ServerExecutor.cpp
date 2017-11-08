#include <ServerExecutor.h>
#include <PollLoopBase.h>
#include <NetworkUtils.h>

#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>


int ServerExecutor::init(PollLoopBase *srv)
{
    this->loop = srv;
    log = loop->log;
    return 0;
}


int ServerExecutor::up(ExecutorData &data)
{
    data.removeOnTimeout = false;
    data.state = ExecutorData::State::ok;

    data.fd0 = socketListen(data.port, log);
    if(data.fd0 < 0)
    {
        return -1;
    }

    if(loop->addPollFd(data, data.fd0, EPOLLIN) != 0)
    {
        return -1;
    }

    return 0;
}


ProcessResult ServerExecutor::process(ExecutorData &data, int fd, int /*events*/)
{
    if(fd != data.fd0)
    {
        log->error("invalid file\n");
        return ProcessResult::ok;
    }

    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    int clientSockFd = accept4(data.fd0, (struct sockaddr *)&address, &addrlen, SOCK_NONBLOCK);

    if(clientSockFd == -1)
    {
        log->error("accept failed: %s", strerror(errno));
        return ProcessResult::shutdown;
    }

    log->debug("server accepted connection\n");

    loop->createRequestExecutor(clientSockFd, ExecutorType::request);

    return ProcessResult::ok;
}

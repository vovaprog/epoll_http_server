#include <SslRequestExecutor.h>
#include <PollLoopBase.h>

#include <sys/epoll.h>
#include <openssl/ssl.h>

int SslRequestExecutor::init(PollLoopBase *loop)
{
    this->loop = loop;
    log = loop->log;
    return 0;
}


int SslRequestExecutor::up(ExecutorData &data)
{
    log->debug("SslRequestExecutor up called\n");

    data.removeOnTimeout = true;
    data.connectionType = (int)ConnectionType::ssl;

    data.buffer.init(ExecutorData::REQUEST_BUFFER_SIZE);

    if(sslInit(data) != 0)
    {
        log->warning("SslRequestExecutor::up sslInit failed\n");
        return -1;
    }

    HandleHandshakeResult result = handleHandshake(data, true);

    if(result == HandleHandshakeResult::ok)
    {
        if(loop->addPollFd(data, data.fd0, EPOLLIN) != 0)
        {
            log->warning("SslRequestExecutor::up addPollFd failed\n");
            return -1;
        }
        data.state = ExecutorData::State::readRequest;
    }
    else if(result == HandleHandshakeResult::again)
    {
        data.state = ExecutorData::State::sslHandshake;
    }
    else
    {
        log->warning("SslRequestExecutor::up handleHandshake failed\n");
        return -1;
    }

    return 0;
}


int SslRequestExecutor::sslInit(ExecutorData &data)
{
    SSL *ssl = nullptr;

    ssl = SSL_new(loop->srv->sslCtx);

    if(ssl == NULL)
    {
        log->error("SSL_new failed\n");
        return -1;
    }

    if(SSL_set_fd(ssl, data.fd0) == 0)
    {
        log->error("SSL_set_fd failed\n");
        SSL_shutdown(ssl);
        SSL_free(ssl);
        return -1;
    }

    SSL_set_accept_state(ssl);

    data.ssl = ssl;

    return 0;
}


SslRequestExecutor::HandleHandshakeResult SslRequestExecutor::handleHandshake(ExecutorData &data, bool firstTime)
{
    int ret = SSL_do_handshake(data.ssl);
    if(ret == 1)
    {
        return HandleHandshakeResult::ok;
    }
    int err = SSL_get_error(data.ssl, ret);
    if(err == SSL_ERROR_WANT_WRITE)
    {
        if(firstTime)
        {
            loop->addPollFd(data, data.fd0, EPOLLOUT);
        }
        else
        {
            loop->editPollFd(data, data.fd0, EPOLLOUT);
        }
    }
    else if(err == SSL_ERROR_WANT_READ)
    {
        if(firstTime)
        {
            loop->addPollFd(data, data.fd0, EPOLLIN);
        }
        else
        {
            loop->editPollFd(data, data.fd0, EPOLLIN);
        }
    }
    else
    {
        log->error("SSL_do_handshake error. return: %d   error: %d   errno: %d   strerror: %s\n", ret, err, errno, strerror(errno));
        return HandleHandshakeResult::error;
    }

    return HandleHandshakeResult::again;
}


ProcessResult SslRequestExecutor::process(ExecutorData &data, int fd, int events)
{
    if(data.state == ExecutorData::State::sslHandshake && fd == data.fd0)
    {
        return process_handshake(data);
    }
    if(data.state == ExecutorData::State::readRequest && fd == data.fd0 && events == EPOLLIN)
    {
        return process_readRequest(data);
    }

    log->warning("invalid process call (sslRequest). data.state: %d   fd0: %s   events: %d\n",
                 (int)data.state, (fd == data.fd0) ? "true" : "false" , events);
    return ProcessResult::removeExecutorError;
}



ssize_t SslRequestExecutor::readFd0(ExecutorData &data, void *buf, size_t count, int &errorCode)
{
    int result = SSL_read(data.ssl, buf, count);

    if(result > 0)
    {
        errorCode = 0;
    }
    else
    {
        int error = SSL_get_error(data.ssl, result);

        if(error == SSL_ERROR_WANT_READ)
        {
            errorCode = EAGAIN;
        }
        else if(error == SSL_ERROR_WANT_WRITE)
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
            log->error("SSL_read failed. result: %d   error: %d   errno: %d   strerror: %s\n", result, error, errno, strerror(errno));
        }
    }

    return result;
}


ProcessResult SslRequestExecutor::process_handshake(ExecutorData &data)
{
    HandleHandshakeResult result = handleHandshake(data, false);

    if(result == HandleHandshakeResult::ok)
    {
        if(loop->editPollFd(data, data.fd0, EPOLLIN) != 0)
        {
            log->warning("SslRequestExecutor::process_handshake editPollFd failed\n");
            return ProcessResult::removeExecutorError;
        }
        data.state = ExecutorData::State::readRequest;
        return ProcessResult::ok;
    }
    else if(result == HandleHandshakeResult::again)
    {
        log->debug("SslRequestExecutor::process_handshake again\n");

        ++data.retryCounter;
        return ProcessResult::ok;
    }
    else
    {
        log->warning("SslRequestExecutor::process_handshake handleHandshake failed\n");
        return ProcessResult::removeExecutorError;
    }
}


ProcessResult SslRequestExecutor::process_readRequest(ExecutorData &data)
{
    if(readRequest(data) != 0)
    {
        log->warning("SslRequestExecutor::process_readRequest readRequest failed\n");
        return ProcessResult::removeExecutorError;
    }

    ParseRequestResult parseResult = parseRequest(data);

    if(parseResult == ParseRequestResult::file)
    {
        return setExecutor(data, loop->getExecutor(ExecutorType::sslFile));
    }
    else if(parseResult == ParseRequestResult::uwsgi)
    {
        return setExecutor(data, loop->getExecutor(ExecutorType::sslUwsgi));
    }
    else if(parseResult == ParseRequestResult::invalid)
    {
        log->warning("SslRequestExecutor::process_readRequest parseRequest failed\n");
        return ProcessResult::removeExecutorError;
    }

    return ProcessResult::ok;
}

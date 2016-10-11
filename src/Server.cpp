#include <Server.h>

#include <LogStdout.h>
#include <LogMmap.h>

#include <mutex>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <pthread.h>
#include <climits>


static std::recursive_mutex *sslMutexes = nullptr;
bool Server::sslInited = false;


static void sslIdCallback(CRYPTO_THREADID *pid)
{
    CRYPTO_THREADID_set_numeric(pid, (unsigned long)pthread_self());
}


static void sslLockCallback(int mode, int type, const char *file, int line)
{
    (void)file;
    (void)line;
    if(mode & CRYPTO_LOCK)
    {
        sslMutexes[type].lock();
    }
    else
    {
        sslMutexes[type].unlock();
    }
}


int Server::sslInit()
{
    if(!sslInited)
    {
        SSL_library_init();
        SSL_load_error_strings();

        sslMutexes = new std::recursive_mutex[CRYPTO_num_locks()];

        CRYPTO_THREADID_set_callback(sslIdCallback);
        CRYPTO_set_locking_callback(sslLockCallback);

        sslInited = true;
    }

    return 0;
}


void Server::cleanup()
{
    if(sslInited)
    {
        sslInited = false;

        CRYPTO_set_locking_callback(NULL);
        CRYPTO_set_id_callback(NULL);

        if(sslMutexes != nullptr)
        {
            delete[] sslMutexes;
            sslMutexes = nullptr;
        }
    }
}


SSL_CTX* Server::sslCreateContext(Log *log)
{
    SSL_CTX *sslCtx = nullptr;

    sslCtx = SSL_CTX_new(SSLv23_method());
    if(sslCtx == NULL)
    {
        log->error("SSL_CTX_new failed\n");
        return nullptr;
    }

    if(SSL_CTX_use_certificate_file(sslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        log->error("SSL_CTX_use_certificate_file failed\n");
        SSL_CTX_free(sslCtx);
        return nullptr;
    }

    if(SSL_CTX_use_PrivateKey_file(sslCtx, "server.pem", SSL_FILETYPE_PEM) <= 0)
    {
        log->error("SSL_CTX_use_PrivateKey_file failed\n");
        SSL_CTX_free(sslCtx);
        return nullptr;
    }

    if(SSL_CTX_check_private_key(sslCtx) <= 0)
    {
        log->error("SSL_CTX_check_private_key\n");
        SSL_CTX_free(sslCtx);
        return nullptr;
    }

    log->info("openssl context inited\n");

    return sslCtx;
}


void Server::sslDestroyContext(SSL_CTX *sslCtx)
{
    SSL_CTX_free(sslCtx);
}


int Server::start(ServerParameters &parameters)
{
    this->parameters = parameters;


    if(parameters.logType == Log::Type::stdout)
    {
        log = new LogStdout();
    }
    else
    {
        log = new LogMmap();
    }

    if(log->init(&parameters) != 0)
    {
        stop();
        return -1;
    }

    parameters.writeToLog(log);

    if(parameters.httpsPorts.size() > 0)
    {
        if(sslInit() != 0)
        {
            stop();
            return -1;
        }

        sslCtx = sslCreateContext(log);

        if(sslCtx == nullptr)
        {
            stop();
            return -1;
        }
    }


    loops = new PollLoop[parameters.threadCount];

    for(int i = 0; i < parameters.threadCount; ++i)
    {
        if(loops[i].init(this, &parameters) != 0)
        {
            stop();
            return -1;
        }
    }

    //=================================================

    int i = 0;
    for(int port : parameters.httpPorts)
    {
        if(loops[i % parameters.threadCount].listenPort(port, ExecutorType::server) != 0)
        {
            stop();
            return -1;
        }
        ++i;
    }
    for(int port : parameters.httpsPorts)
    {
        if(loops[i % parameters.threadCount].listenPort(port, ExecutorType::serverSsl) != 0)
        {
            stop();
            return -1;
        }
        ++i;
    }

    //=================================================

    threads = new std::thread[parameters.threadCount];

    for(int i = 0; i < parameters.threadCount; ++i)
    {
        threads[i] = std::thread(&PollLoop::run, &loops[i]);
    }

    return 0;
}


void Server::stop()
{
    if(loops != nullptr)
    {
        for(int i = 0; i < parameters.threadCount; ++i)
        {
            loops[i].stop();
        }
    }

    if(threads != nullptr)
    {
        for(int i = 0; i < parameters.threadCount; ++i)
        {
            threads[i].join();
        }
    }

    if(loops != nullptr)
    {
        delete[] loops;
        loops = nullptr;
    }

    if(threads != nullptr)
    {
        delete[] threads;
        threads = nullptr;
    }

    if(sslCtx != nullptr)
    {
        sslDestroyContext(sslCtx);
        sslCtx = nullptr;
    }

    if(log != nullptr)
    {
        delete log;
        log = nullptr;
    }
}


int Server::createRequestExecutor(int fd, ExecutorType execType)
{
    int minPollFds = INT_MAX;
    int minIndex = 0;

    for(int i = 0; i < parameters.threadCount; ++i)
    {
        int numberOfFds = loops[i].numberOfPollFds();
        if(numberOfFds < minPollFds)
        {
            minPollFds = numberOfFds;
            minIndex = i;
        }
    }

    if(loops[minIndex].enqueueClientFd(fd, execType) != 0)
    {
        log->error("enqueueClientFd failed\n");
        close(fd);
        return -1;
    }

    return 0;
}


void Server::logStats()
{
    int totalNumberOfFds = 0;

    for(int i = 0; i < parameters.threadCount; ++i)
    {
        int numberOfFds = loops[i].numberOfPollFds();
        totalNumberOfFds += numberOfFds;
        log->info("poll files. thread %d: %d\n", i, numberOfFds);
    }

    log->info("poll files. total:    %d\n", totalNumberOfFds);
}


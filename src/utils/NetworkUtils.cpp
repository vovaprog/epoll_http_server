#include "NetworkUtils.h"

#include <Log.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/un.h>


int socketConnectNonBlock(const char *address, int port, bool &connected, Log *log)
{
    connected = false;

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if(sock < 0)
    {
        log->error("socket failed: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in remoteaddr;
    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_addr.s_addr = inet_addr(address);
    remoteaddr.sin_port = htons(port);

    if(connect(sock, (struct sockaddr*)&remoteaddr, sizeof(remoteaddr)) != 0)
    {
        if(errno != EINPROGRESS)
        {
            log->error("connect failed: %s\n", strerror(errno));
            close(sock);
            return -1;
        }
    }
    else
    {
        connected = true;
    }

    return sock;
}


int socketConnectUnixNonBlock(const char *path, bool &connected, Log *log)
{
    connected = false;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(sockaddr_un));

    addr.sun_family = AF_UNIX;
    if(snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path) >= (int)sizeof(addr.sun_path))
    {
        log->error("socket name is too long\n");
        return -1;
    }

    int sock = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if(sock < 0)
    {
        log->error("socket failed: %s\n", strerror(errno));
        return -1;
    }

    if(connect(sock, (struct sockaddr*)&addr, sizeof(sockaddr_un)) != 0)
    {
        if(errno != EINPROGRESS)
        {
            log->error("connect failed: %s\n", strerror(errno));
            close(sock);
            return -1;
        }
    }
    else
    {
        connected = true;
    }

    return sock;
}


int socketConnectNonBlockCheck(int fd, Log *log)
{
    int socketError = 0;
    socklen_t len = sizeof(socketError);

    if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &len) != 0)
    {
        log->error("getsockopt failed: %s\n", strerror(errno));
        return errno;
    }

    return socketError;
}


int socketListen(int port, Log *log)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0)
    {
        log->error("socket failed: %s\n", strerror(errno));
        return -1;
    }

    int enable = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0)
    {
        log->error("setsockopt failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in)) != 0)
    {
        log->error("bind failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    if(listen(sockfd, 1000) != 0) //length of queue of pending connections
    {
        log->error("listen failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int setNonBlock(int fd, Log *log)
{
    int flags;

    if((flags = fcntl(fd, F_GETFL, 0)) < 0)
    {
        log->error("fcntl failed: %s\n", strerror(errno));
        return -1;
    }


    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        log->error("fcntl failed: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


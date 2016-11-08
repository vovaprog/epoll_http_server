#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

class Log;

int socketConnectNonBlock(const char *address, int port, bool &connected, Log *log);
int socketConnectNonBlockCheck(int fd, Log *log);

int socketListen(int port, Log *log);

int setNonBlock(int fd, Log *log);

#endif


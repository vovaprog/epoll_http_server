#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

class Log;

int readBytes(int fd, char *buf, int numberOfBytes);
int writeBytes(int fd, char *buf, int numberOfBytes);

int socketConnect(const char *address, int port, Log *log);

int socketListen(int port, Log *log);

int setNonBlock(int fd, Log *log);

#endif


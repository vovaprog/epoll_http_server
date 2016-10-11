#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

int readBytes(int fd, char *buf, int numberOfBytes);
int writeBytes(int fd, char *buf, int numberOfBytes);

int socketConnect(const char *address, int port);

int socketListen(int port);

int setNonBlock(int fd);

#endif


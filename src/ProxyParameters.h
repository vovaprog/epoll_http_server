#ifndef PROXY_PARAMETERS_H
#define PROXY_PARAMETERS_H

#include <ConnectionType.h>
#include <SocketType.h>

#include <string>

struct ProxyParameters
{
    std::string prefix;
    std::string address;
    int port;

    // is declared int, because bitwise operations are performed with value
    int connectionType = (int)ConnectionType::none;

    SocketType socketType = SocketType::none;
};

#endif

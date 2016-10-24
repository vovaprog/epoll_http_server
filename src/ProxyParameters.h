#ifndef PROXY_PARAMETERS_H
#define PROXY_PARAMETERS_H

#include <ConnectionType.h>

#include <string>

struct ProxyParameters
{
    std::string prefix;
    std::string address;
    int port;
    int connectionType = (int)ConnectionType::none;
};

#endif

#ifndef UWSGI_APPLICATION_PARAMETERS
#define UWSGI_APPLICATION_PARAMETERS

#include <ConnectionType.h>

#include <string>

struct UwsgiApplicationParameters
{
    std::string prefix;
    int port;
    int connectionType = (int)ConnectionType::none;
};

#endif

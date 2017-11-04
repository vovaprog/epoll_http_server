#ifndef CONNECTION_TYPE_H
#define CONNECTION_TYPE_H

enum class ConnectionType
{
    none = 0, clear = 1,

#ifdef USE_SSL
    ssl = 2
#endif
};

#endif

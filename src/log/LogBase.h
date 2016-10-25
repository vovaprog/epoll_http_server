#ifndef _LOG_BASE_H_
#define _LOG_BASE_H_

#include <Log.h>
#include <ServerParameters.h>

class LogBase: public Log
{
public:
    int init(ServerParameters *params) override;

    void debug(const char* format, ...) override __attribute__((format(printf, 2, 3)));

    void info(const char* format, ...) override __attribute__((format(printf, 2, 3)));

    void warning(const char* format, ...) override __attribute__((format(printf, 2, 3)));

    void error(const char* format, ...) override __attribute__((format(printf, 2, 3)));

    void writeLog(Level argLevel, const char *format, ...) override __attribute__((format(printf, 3, 4)));

protected:

    virtual void writeLog(const char *prefix, const char* format, va_list args) = 0;

};

#endif

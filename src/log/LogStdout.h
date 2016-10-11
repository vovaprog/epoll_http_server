#ifndef _LOG_STDOUT_H_
#define _LOG_STDOUT_H_

#include <LogBase.h>

#include <cstdarg>
#include <mutex>

class LogStdout: public LogBase
{
protected:

    void writeLog(const char *prefix, const char* format, va_list args) override;

protected:

    std::mutex logMtx;
};


#endif

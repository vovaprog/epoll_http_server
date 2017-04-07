#include <LogBase.h>

#include <cstdarg>

#define debugPrefix   "[DEBUG]  "
#define infoPrefix    "[INFO]   "
#define warningPrefix "[WARNING]"
#define errorPrefix   "[ERROR]  "


int LogBase::init(ServerParameters *params)
{
    level = params->logLevel;
    return 0;
}


void LogBase::debug(const char* format, ...)
{
    if(level <= Level::debug)
    {
        va_list args;
        va_start(args, format);

        writeLog(debugPrefix, format, args);

        va_end(args);
    }
}


void LogBase::info(const char* format, ...)
{
    if(level <= Level::info)
    {
        va_list args;
        va_start(args, format);
        writeLog(infoPrefix, format, args);
        va_end(args);
    }
}


void LogBase::warning(const char* format, ...)
{
    if(level <= Level::warning)
    {
        va_list args;
        va_start(args, format);
        writeLog(warningPrefix, format, args);
        va_end(args);
    }
}


void LogBase::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    writeLog(errorPrefix, format, args);
    va_end(args);
}


void LogBase::writeLog(Level argLevel, const char *format, ...)
{
    if(argLevel >= level)
    {
        const char *prefix = errorPrefix;

        switch(argLevel){
        case Level::debug:
            prefix = debugPrefix;
            break;
        case Level::info:
            prefix = infoPrefix;
            break;
        case Level::warning:
            prefix = warningPrefix;
            break;
        case Level::error:
            prefix = errorPrefix;
            break;
        }

        va_list args;
        va_start(args, format);

        writeLog(prefix, format, args);

        va_end(args);
    }
}

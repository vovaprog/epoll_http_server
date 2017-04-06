#include <LogBase.h>

#include <cstdarg>

#define debugPrefix   "[DEBUG]  "
#define infoPrefix    "[INFO]   "
#define warningPrefix "[WARNING]"
#define errorPrefix   "[ERROR]  "

int LogBase::init(ServerParameters *params)
{
    this->level = params->logLevel;
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
    if(argLevel == Level::debug && level <= Level::debug)
    {
        va_list args;
        va_start(args, format);

        writeLog(debugPrefix, format, args);

        va_end(args);
    }
    else if(argLevel == Level::info && level <= Level::info)
    {
        va_list args;
        va_start(args, format);

        writeLog(infoPrefix, format, args);

        va_end(args);
    }
    else if(argLevel == Level::warning && level <= Level::warning)
    {
        va_list args;
        va_start(args, format);

        writeLog(warningPrefix, format, args);

        va_end(args);
    }
    else if(argLevel == Level::error && level <= Level::error)
    {
        va_list args;
        va_start(args, format);

        writeLog(errorPrefix, format, args);

        va_end(args);
    }
}

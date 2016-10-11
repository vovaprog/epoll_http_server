#include <LogBase.h>

#include <cstdarg>


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

        writeLog("[DEBUG]  ", format, args);

        va_end(args);
    }
}


void LogBase::info(const char* format, ...)
{
    if(level <= Level::info)
    {
        va_list args;
        va_start(args, format);
        writeLog("[INFO]   ", format, args);
        va_end(args);
    }
}


void LogBase::warning(const char* format, ...)
{
    if(level <= Level::warning)
    {
        va_list args;
        va_start(args, format);
        writeLog("[WARNING]", format, args);
        va_end(args);
    }
}


void LogBase::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    writeLog("[ERROR]  ", format, args);
    va_end(args);
}


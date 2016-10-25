#ifndef _LOG_H_
#define _LOG_H_

struct ServerParameters;

class Log
{
public:
    enum class Level
    {
        debug = 1, info = 2, warning = 3, error = 4
    };

    enum class Type
    {
        stdout, mmap
    };

    virtual ~Log() { }

    virtual int init(ServerParameters *params) = 0;

    virtual void debug(const char* format, ...)   __attribute__((format(printf, 2, 3))) = 0;
    virtual void info(const char* format, ...)    __attribute__((format(printf, 2, 3))) = 0;
    virtual void warning(const char* format, ...) __attribute__((format(printf, 2, 3))) = 0;
    virtual void error(const char* format, ...)   __attribute__((format(printf, 2, 3))) = 0;

    virtual void writeLog(Level argLevel, const char *format, ...) __attribute__((format(printf, 3, 4))) = 0;

    static const char* logLevelString(Level level)
    {
        switch(level)
        {
        case Level::debug:
            return "debug";
        case Level::info:
            return "info";
        case Level::warning:
            return "warning";
        case Level::error:
            return "error";
        default:
            return "unknown";
        }
    }

    static const char* logTypeString(Type type)
    {
        switch(type)
        {
        case Type::stdout:
            return "stdout";
        case Type::mmap:
            return "mmap";
        default:
            return "unknown";
        }
    }

protected:

    Level level = Level::info;
};


#endif

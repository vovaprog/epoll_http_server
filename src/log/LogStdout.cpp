#include <LogStdout.h>
#include <TimeUtils.h>

void LogStdout::writeLog(const char *prefix, const char* format, va_list args)
{
    std::lock_guard<std::mutex> lock(logMtx);

    char timeBuffer[50];

    getCurrentTimeString(timeBuffer, 50);

    printf("%s %s   ", prefix, timeBuffer);
    vprintf(format, args);
}


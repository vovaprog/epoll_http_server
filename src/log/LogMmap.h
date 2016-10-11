#ifndef _LOG_MMAP_H_
#define _LOG_MMAP_H_

#include <LogBase.h>
#include <ServerParameters.h>
#include <TransferBuffer.h>

#include <mutex>

class LogMmap: public LogBase
{
public:
    ~LogMmap() override
    {
        closeFile();
    }

    int init(ServerParameters *params) override;


protected:

    int openFile();

    void closeFile();

    int rotate();

    void writeLog(const char *prefix, const char* format, va_list args) override;


protected:

    std::mutex logMtx;

    int logFileSize = 0;
    int logArchiveCount = 0;

    TransferBuffer buffer;

    const int maxMessageSize = 512;

    int fd = -1;

    static const int maxLogFileNameSize = 300;
    char logFileName[maxLogFileNameSize + 1];
};


#endif

#include <LogMmap.h>
#include <TimeUtils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <utility>


int LogMmap::init(ServerParameters *params)
{
    if(params->logFolder.size() + 2 > (unsigned int)maxLogFileNameSize - 30)
    {
        return -1;
    }

    int ret = LogBase::init(params);

    if(ret != 0)
    {
        return ret;
    }

    logFileSize = params->logFileSize;
    logArchiveCount = params->logArchiveCount;

    strcpy(logFileName, params->logFolder.c_str());


    struct stat st;

    if(stat(logFileName, &st) == -1)
    {
        if(mkdir(logFileName, 0700) != 0)
        {
            perror("mkdir failed");
            return -1;
        }
    }

    strcat(logFileName, "/http.log");

    return rotate();
}


int LogMmap::openFile()
{
    fd = open(logFileName, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fd < 0)
    {
        perror("open failed");
        return -1;
    }

    if(ftruncate(fd, logFileSize) != 0)
    {
        perror("ftruncate failed");

        close(fd);
        fd = -1;

        return -1;
    }

    void *p = mmap(NULL, logFileSize, PROT_WRITE, MAP_SHARED, fd, 0);
    if(p == MAP_FAILED)
    {
        perror("mmap failed");

        close(fd);
        fd = -1;

        return -1;
    }

    buffer.init(p, logFileSize);

    return 0;
}


void LogMmap::closeFile()
{
    if(fd >= 0)
    {
        void *ptr = buffer.getDataPtr();

        buffer.init(nullptr, 0);

        if(ptr != nullptr)
        {
            if(munmap(ptr, logFileSize) != 0)
            {
                perror("munmap failed\n");
            }
        }

        close(fd);
        fd = -1;
    }
}


int LogMmap::rotate()
{
    closeFile();

    char fileName0[301];
    char fileName1[301];
    char *p0 = fileName0;
    char *p1 = fileName1;

    snprintf(p1, 300, "%s.%d", logFileName, logArchiveCount);
    remove(p1);

    for(int i = logArchiveCount - 1; i > 0; --i)
    {
        snprintf(p0, 300, "%s.%d", logFileName, i);
        rename(p0, p1);
        std::swap(p0, p1);
    }

    rename(logFileName, p1);

    return openFile();
}


void LogMmap::writeLog(const char *prefix, const char* format, va_list args)
{
    std::lock_guard<std::mutex> lock(logMtx);

    void *data;
    int size;

    bool ret = buffer.startWrite(data, size);

    if(ret == false || (ret == true && size < maxMessageSize))
    {
        if(rotate() != 0)
        {
            return;
        }
        if(!buffer.startWrite(data, size))
        {
            return;
        }
        else if(size < maxMessageSize)
        {
            return;
        }
    }

    char timeBuffer[50];

    getCurrentTimeString(timeBuffer, 50);

    int bytesToWrite = maxMessageSize;
    int prefixLength = snprintf(static_cast<char*>(data), bytesToWrite, "%s %s   ", prefix, timeBuffer);

    if(prefixLength >= bytesToWrite)
    {
        // snprintf adds zero char if output is not big enough.
        // bytesToWrite includes zero char. decrement it not to write zero to output.
        int bytesWritten = bytesToWrite - 1;
        buffer.endWrite(bytesWritten);
    }
    else
    {
        bytesToWrite = maxMessageSize - prefixLength;
        int bytesWritten = vsnprintf(static_cast<char*>(data) + prefixLength, bytesToWrite, format, args);

        if(bytesWritten >= bytesToWrite)
        {
            // snprintf adds zero char if output is not big enough.
            // bytesToWrite includes zero char. decrement it not to write zero to output.
            bytesWritten = bytesToWrite - 1;
        }

        buffer.endWrite(prefixLength + bytesWritten);
    }
}

#include <LogMmap.h>
#include <TimeUtils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>


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


    struct stat st = { 0 };

    if (stat(logFileName, &st) == -1)
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
        return -1;
    }

    void *p = mmap(NULL, logFileSize, PROT_WRITE, MAP_SHARED, fd, 0);
    if(p == MAP_FAILED)
    {
        perror("mmap failed");
        return -1;
    }

    buffer.init(p, logFileSize);

    return 0;
}


void LogMmap::closeFile()
{
    if(fd >= 0)
    {
        buffer.init(nullptr, 0);

        if(munmap(buffer.getDataPtr(), logFileSize) != 0)
        {
            perror("munmap failed\n");
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

    int prefixLength = sprintf((char*)data, "%s %s   ", prefix, timeBuffer);
    int bytesWritten = vsnprintf((char*)data + prefixLength, maxMessageSize - prefixLength, format, args);

    buffer.endWrite(prefixLength + bytesWritten);
}

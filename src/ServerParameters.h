#ifndef SERVER_PARAMETERS_H
#define SERVER_PARAMETERS_H

#include <Log.h>
#include <vector>
#include <string>
#include <ProxyParameters.h>

struct ServerParameters
{
    ServerParameters()
    {
        setDefaults();
    }

    void setDefaults()
    {
        rootFolder = "./data";
        logFolder = "./log";
        threadCount = 1;
        httpPorts.clear();
        httpsPorts.clear();
        proxies.clear();
        logLevel = Log::Level::info;
        logType = Log::Type::stdout;
        logFileSize = 1024 * 1024;
        logArchiveCount = 10;
        executorTimeoutMillis = 10000;
        logStats = true;
        maxAllocationBlocks = 1000;
        allocationBlockSize = 100;
    }

    int load(const char *fileName);

    void writeToLog(Log *log) const;


    std::string rootFolder;
    std::string logFolder;

    int threadCount;
    int executorTimeoutMillis;

    Log::Level logLevel;
    Log::Type logType;
    int logFileSize;
    int logArchiveCount;

    bool logStats;

    int maxAllocationBlocks;
    int allocationBlockSize;

    std::vector<int> httpPorts;
    std::vector<int> httpsPorts;
    std::vector<ProxyParameters> proxies;
};

#endif


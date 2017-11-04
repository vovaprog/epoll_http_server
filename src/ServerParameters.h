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

#if USE_SSL
        httpsPorts.clear();
#endif

        proxies.clear();
        logLevel = Log::Level::info;
        logType = Log::Type::stdout;
        logFileSize = 1024 * 1024;
        logArchiveCount = 10;
        executorTimeoutMillis = 10000;
        logStats = true;
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

    std::vector<int> httpPorts;

#if USE_SSL
    std::vector<int> httpsPorts;
#endif

    std::vector<ProxyParameters> proxies;
};

#endif


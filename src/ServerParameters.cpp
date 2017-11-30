#include <ServerParameters.h>

#include <ConfigReader.h>


template<typename ParamType>
bool getOptionalInt(const ConfigMapType &params, const char *paramName, ParamType &param)
{
    auto iter = params.find(paramName);
    if (iter != params.end())
    {
        try
        {
            param = std::stoi(iter->second);
        }
        catch (...)
        {
            printf("invalid parameter value: %s", paramName);
            return false;
        }
    }
    return true;
}


int ServerParameters::load(const char *fileName)
{
    setDefaults();

    ConfigMapType configMap;
    if (!configReadFile(fileName, configMap))
    {
        printf("read config failed\n");
        return -1;
    }

    if (!getOptionalInt(configMap, "threadCount", threadCount))
    {
        return -1;
    }
    if (!getOptionalInt(configMap, "executorTimeoutMillis", executorTimeoutMillis))
    {
        return -1;
    }
    if (!getOptionalInt(configMap, "logFileSize", logFileSize))
    {
        return -1;
    }
    if (!getOptionalInt(configMap, "logArchiveCount", logArchiveCount))
    {
        return -1;
    }

    auto iter = configMap.find("rootFolder");
    if (iter != configMap.end())
    {
        rootFolder = iter->second;
    }

    iter = configMap.find("logFolder");
    if (iter != configMap.end())
    {
        rootFolder = iter->second;
    }

    iter = configMap.find("logLevel");
    if (iter != configMap.end())
    {
        if (iter->second == "debug") logLevel = Log::Level::debug;
        else if (iter->second == "info") logLevel = Log::Level::info;
        else if (iter->second == "warning") logLevel = Log::Level::warning;
        else if (iter->second == "error") logLevel = Log::Level::error;
        else
        {
            printf("invalid logLevel\n");
            return -1;
        }
    }

    iter = configMap.find("logType");
    if (iter != configMap.end())
    {
        if (iter->second == "stdout") logType = Log::Type::stdout;
        else if (iter->second == "mmap") logType = Log::Type::mmap;
        else
        {
            printf("invalid logType\n");
            return -1;
        }
    }

    for (int portNum = 0; portNum < 100; ++portNum)
    {
        std::string key = "httpPort" + std::to_string(portNum);

        int port = -1;
        if (!getOptionalInt(configMap, key.c_str(), port))
        {
            return -1;
        }
        if (port < 0)
        {
            break;
        }
        httpPorts.push_back(port);
    }

    if (httpPorts.size() < 1)
    {
        printf("http port is not specified\n");
        return -1;
    }

    // httpsPort parameter is not implemented.

    for (int proxyNum = 0; proxyNum < 100; ++proxyNum)
    {
        std::string key = "proxy" + std::to_string(proxyNum) + ".port";

        ProxyParameters proxy;

        if (!getOptionalInt(configMap, key.c_str(), proxy.port))
        {
            return -1;
        }
        if (proxy.port <= 0)
        {
            break;
        }

        key = "proxy" + std::to_string(proxyNum) + ".address";
        iter = configMap.find(key);
        if (iter == configMap.end())
        {
            printf("proxy address is not set\n");
            return -1;
        }
        proxy.address = iter->second;

        key = "proxy" + std::to_string(proxyNum) + ".prefix";
        iter = configMap.find(key);
        if (iter == configMap.end())
        {
            printf("proxy prefix is not set\n");
            return -1;
        }
        proxy.prefix = iter->second;

        key = "proxy" + std::to_string(proxyNum) + ".socket";
        iter = configMap.find(key);
        if (iter != configMap.end())
        {
            if (iter->second == "tcp")
            {
                proxy.socketType = SocketType::tcp;
            }
            else if (iter->second == "unix")
            {
                proxy.socketType = SocketType::unix;
            }
            else
            {
                printf("invalid proxy socket type\n");
                return -1;
            }
        }
        else
        {
            proxy.socketType = SocketType::tcp;
        }

        // connectionType parameter is not implemented, just set it to clear.
        proxy.connectionType = static_cast<int>(ConnectionType::clear);

        proxies.push_back(proxy);
    }

    return 0;
}


void ServerParameters::writeToLog(Log *log) const
{
    log->info("----- server parameters -----\n");
    log->info("rootFolder: %s\n", rootFolder.c_str());
    log->info("threadCount: %d\n", threadCount);
    log->info("executorTimeoutMillis: %d\n", executorTimeoutMillis);
    log->info("logLevel: %s\n", Log::logLevelString(logLevel));
    log->info("logType: %s\n", Log::logTypeString(logType));
    log->info("logFileSize: %d\n", logFileSize);
    log->info("logArchiveCount: %d\n", logArchiveCount);
    for (int port : httpPorts)
    {
        log->info("httpPort: %d\n", port);
    }

#if USE_SSL
    for (int port : httpsPorts)
    {
        log->info("httpsPort: %d\n", port);
    }
#endif

    for (const ProxyParameters & proxy : proxies)
    {
        const char *conTypeString = "none";

#if USE_SSL
        if ((proxy.connectionType & (int)ConnectionType::clear) && (proxy.connectionType & (int)ConnectionType::ssl))
        {
            conTypeString = "clear, ssl";
        }
        else if (proxy.connectionType & (int)ConnectionType::clear)
        {
            conTypeString = "clear";
        }
        else if (proxy.connectionType & (int)ConnectionType::ssl)
        {
            conTypeString = "ssl";
        }
#else
        if (proxy.connectionType & (int)ConnectionType::clear)
        {
            conTypeString = "clear";
        }
#endif

        const char *socketTypeString = "none";

        if (proxy.socketType == SocketType::tcp)
        {
            socketTypeString = "tcp";
        }
        else if (proxy.socketType == SocketType::unix)
        {
            socketTypeString = "unix";
        }

        log->info("proxy   prefix: %s   address: %s   port: %d   connection: %s   socket: %s\n",
                  proxy.prefix.c_str(), proxy.address.c_str(), proxy.port, conTypeString, socketTypeString);
    }
    log->info("-----------------------------\n");
}



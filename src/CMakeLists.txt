cmake_minimum_required(VERSION 2.8)

project (epoll_http_server)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("." "./log" "./executors" "./utils")

set(SOURCE 
    main.cpp

    ServerBase.h
    Server.h     Server.cpp

    PollLoopBase.h
    PollLoop.h     PollLoop.cpp

    HttpRequest.h HttpRequest.cpp
    HttpResponse.h HttpResponse.cpp

    ProxyParameters.h
    ProcessResult.h
    ExecutorType.h
    ConnectionType.h
    PollData.h
    ExecutorData.h     ExecutorData.cpp
    ServerParameters.h ServerParameters.cpp

    log/Log.h
    log/LogBase.h   log/LogBase.cpp
    log/LogMmap.h   log/LogMmap.cpp
    log/LogStdout.h log/LogStdout.cpp

    executors/Executor.h           executors/Executor.cpp
    executors/ServerExecutor.h     executors/ServerExecutor.cpp
    executors/SslServerExecutor.h  executors/SslServerExecutor.cpp
    executors/RequestExecutor.h    executors/RequestExecutor.cpp
    executors/FileExecutor.h       executors/FileExecutor.cpp
    executors/NewFdExecutor.h      executors/NewFdExecutor.cpp
    executors/SslRequestExecutor.h executors/SslRequestExecutor.cpp
    executors/SslFileExecutor.h    executors/SslFileExecutor.cpp
    executors/SslProxyExecutor.h   executors/SslProxyExecutor.cpp
    executors/ProxyExecutor.h      executors/ProxyExecutor.cpp

    utils/TransferRingBuffer.h
    utils/TransferBuffer.h
    utils/FileUtils.h          utils/FileUtils.cpp
    utils/NetworkUtils.h       utils/NetworkUtils.cpp
    utils/TimeUtils.h          utils/TimeUtils.cpp
    utils/SslUtils.h           utils/SslUtils.cpp)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -pedantic -pthread")

add_executable(epoll_http_server ${SOURCE})

target_link_libraries(epoll_http_server ssl crypto tinyxml2)

add_executable(test_http_request ../tests/TestHttpRequest.cpp HttpRequest.h HttpRequest.cpp)



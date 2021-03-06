cmake_minimum_required(VERSION 2.8)

project (epoll_http_server)

set(CMAKE_VERBOSE_MAKEFILE on)

include_directories("." "./log" "./executors" "./utils")

option(USE_SSL "build with ssl (https) support" OFF)

if(${USE_SSL})
    message(STATUS "USE_SSL=ON")

    add_definitions(-DUSE_SSL)
    set(SOURCE_SSL
        executors/SslServerExecutor.h      executors/SslServerExecutor.cpp
        executors/SslRequestExecutor.h     executors/SslRequestExecutor.cpp
        executors/SslFileExecutor.h        executors/SslFileExecutor.cpp
        executors/SslProxyExecutor.h       executors/SslProxyExecutor.cpp
        utils/SslUtils.h                   utils/SslUtils.cpp)

    set(SSL_LINK_LIB ssl crypto)

else()
    message(STATUS "USE_SSL=OFF")

    set(SOURCE_SSL "")
    set(SSL_LINK_LIB "")
endif()

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
    SocketType.h
    PollData.h
    ExecutorData.h     ExecutorData.cpp
    ServerParameters.h ServerParameters.cpp

    log/Log.h
    log/LogBase.h   log/LogBase.cpp
    log/LogMmap.h   log/LogMmap.cpp
    log/LogStdout.h log/LogStdout.cpp

    executors/Executor.h               executors/Executor.cpp
    executors/ServerExecutor.h         executors/ServerExecutor.cpp
    executors/RequestExecutor.h        executors/RequestExecutor.cpp
    executors/FileExecutor.h           executors/FileExecutor.cpp
    executors/NewFdExecutor.h          executors/NewFdExecutor.cpp
    executors/ProxyExecutorReadWrite.h executors/ProxyExecutorReadWrite.cpp
    executors/ProxyExecutorSplice.h    executors/ProxyExecutorSplice.cpp

    utils/TransferRingBuffer.h
    utils/TransferBuffer.h
    utils/BlockStorage.h
    utils/NetworkUtils.h       utils/NetworkUtils.cpp
    utils/TimeUtils.h          utils/TimeUtils.cpp
    utils/ConfigReader.h       utils/ConfigReader.cpp

    ${SOURCE_SSL})


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pedantic-errors -pthread -fstack-protector")

add_executable(epoll_http_server ${SOURCE})

target_link_libraries(epoll_http_server ${SSL_LINK_LIB})

add_executable(test_http_request ../tests/TestHttpRequest.cpp HttpRequest.h HttpRequest.cpp)
add_executable(test_block_storage ../tests/TestBlockStorage.cpp utils/BlockStorage.h)



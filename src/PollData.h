#ifndef POLL_DATA_H
#define POLL_DATA_H

#include <BlockStorage.h>

struct ExecutorData;

struct PollData
{
    int fd = -1;
    ExecutorData *execData = nullptr;

    BlockStorage<PollData>::ServiceData bsData;
};


#endif


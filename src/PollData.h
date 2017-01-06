#ifndef POLL_DATA_H
#define POLL_DATA_H

#include <ListStorage.h>

struct ExecutorData;

struct PollData
{
    PollData() = default;

    PollData(const PollData &pd) = delete;
    PollData(PollData &&pd) = delete;
    PollData& operator=(const PollData &pd) = delete;
    PollData& operator=(PollData && pd) = delete;

    void down()
    {
        fd = -1;
        execData = nullptr;
    }

    int fd = -1;
    ExecutorData *execData = nullptr;

    ListStorage<PollData>::ServiceData listStorageData;
};


#endif


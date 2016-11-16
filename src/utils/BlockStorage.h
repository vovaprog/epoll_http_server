#ifndef BLOCK_STORAGE_H
#define BLOCK_STORAGE_H

#include <vector>
#include <set>

template<class T>
class BlockStorage
{
public:

    BlockStorage() = default;

    BlockStorage(const BlockStorage &bs) = delete;
    BlockStorage(BlockStorage &&bs) = delete;
    BlockStorage& operator=(const BlockStorage &bs) = delete;
    BlockStorage& operator=(BlockStorage && bs) = delete;


    ~BlockStorage()
    {
        destroy();
    }


    int init(int maxBlocks, int blockSize)
    {
        this->maxBlocks = maxBlocks;
        this->blockSize = blockSize;

        nextBlock = 0;

        data = new T*[maxBlocks];

        for(int i = 0; i < maxBlocks; ++i)
        {
            data[i] = nullptr;
        }

        empty.clear();
        used.clear();

        return 0;
    }


    void destroy()
    {
        if(data != nullptr)
        {
            for(int i = 0; i < maxBlocks; ++i)
            {
                if(data[i] != nullptr)
                {
                    delete[] data[i];
                    data[i] = nullptr;
                }
            }

            delete[] data;
            data = nullptr;
        }

        empty.clear();
        used.clear();

        nextBlock = 0;
        maxBlocks = 0;
        blockSize = 0;
    }


    T* get(int index) const
    {
        int blockIndex = index / blockSize;
        int itemIndex = index % blockSize;

        if(blockIndex >= maxBlocks || blockIndex < 0)
        {
            return nullptr;
        }
        if(data[blockIndex] == nullptr)
        {
            return nullptr;
        }

        return &data[blockIndex][itemIndex];
    }


    T* allocate()
    {
        if(empty.size() <= 0)
        {
            if(allocateBlock() != 0)
            {
                return nullptr;
            }
            if(empty.size() <= 0)
            {
                return nullptr;
            }
        }

        int index = empty.back();
        empty.pop_back();
        used.insert(index);

        return get(index);
    }


    int free(int index)
    {
        empty.push_back(index);
        used.erase(index);
        return 0;
    }


    const std::set<int>& usedIndexes() const
    {
        return used;
    }


    struct StorageInfo
    {
        int maxBlocks = 0;
        int blockSize = 0;
        int allocatedBlocks = 0;
        int usedItems = 0;
        int emptyItems = 0;
    };

    int getStorageInfo(StorageInfo &si) const
    {
        si.maxBlocks = maxBlocks;
        si.blockSize = blockSize;
        si.allocatedBlocks = nextBlock;
        si.usedItems = static_cast<int>(used.size());
        si.emptyItems = static_cast<int>(empty.size());

        return 0;
    }

protected:

    int allocateBlock()
    {
        if(nextBlock >= maxBlocks)
        {
            return -1;
        }
        data[nextBlock] = new T[blockSize];

        int index = nextBlock * blockSize;

        for(int i = 0; i < blockSize; ++i)
        {
            data[nextBlock][i].index = index;
            empty.push_back(index);
            ++index;
        }

        ++nextBlock;

        return 0;
    }


protected:

    T **data = nullptr;
    int maxBlocks = 0;
    int blockSize = 0;
    int nextBlock = 0;

    std::vector<int> empty;
    std::set<int> used;
};

#endif




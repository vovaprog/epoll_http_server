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


    struct ServiceData {
        int index = -1;
        bool used = false;
    };
 

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

        nextBlock = 0;
        maxBlocks = 0;
        blockSize = 0;
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

        return get(index);
    }


    int free(T *item)
    {
        empty.push_back(item->bsData.index);
        return 0;
    }


    struct StorageInfo
    {
        int maxBlocks = 0;
        int blockSize = 0;
        int allocatedBlocks = 0;
        int emptyItems = 0;
    };

    int getStorageInfo(StorageInfo &si) const
    {
        si.maxBlocks = maxBlocks;
        si.blockSize = blockSize;
        si.allocatedBlocks = nextBlock;
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
            data[nextBlock][i].bsData.index = index;
            data[nextBlock][i].bsData.used = false;
            empty.push_back(index);
            ++index;
        }

        ++nextBlock;

        return 0;
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

   
protected:

    T **data = nullptr;
    int maxBlocks = 0;
    int blockSize = 0;
    int nextBlock = 0;

    std::vector<int> empty;
};

#endif




#ifndef BLOCK_STORAGE_H
#define BLOCK_STORAGE_H

template <typename T, int blockSize = 0x400>
class BlockStorage
{
public:
    struct ServiceData
    {
        T *next = nullptr;
        T *prev = nullptr;
    };

    BlockStorage() = default;

    BlockStorage(const BlockStorage &fa) = delete;
    BlockStorage(BlockStorage &&fa) = delete;
    BlockStorage& operator=(const BlockStorage &fa) = delete;
    BlockStorage& operator=(BlockStorage && fa) = delete;

    ~BlockStorage()
    {
        destroy();
    }


    void destroy()
    {
        Block *cur = blocks;
        while(cur != nullptr)
        {
            Block *temp = cur->next;
            delete cur;
            cur = temp;
        }
        blocks = nullptr;
        freeListHead = nullptr;
        usedListHead = nullptr;
    }


    T* allocate()
    {
        if(freeListHead == nullptr)
        {
            allocateBlock();
        }

        T *result = freeListHead;
        freeListHead = result->blockStorageData.next;

        addToUsedList(result);

        return result;
    }


    inline void free(T *p)
    {
        removeFromUsedList(p);

        p->blockStorageData.next = freeListHead;
        freeListHead = p;
    }


    inline T* head() const
    {
        return usedListHead;
    }

    inline T* next(const T* item) const
    {
        return item->blockStorageData.next;
    }


    struct StorageInfo
    {
        int allocatedCount = 0;
    };

    int getStorageInfo(StorageInfo &si) const
    {
        T *cur = usedListHead;
        int counter = 0;
        while(cur != nullptr)
        {
            ++counter;
            cur = cur->blockStorageData.next;
        }

        si.allocatedCount = counter;

        return 0;
    }


protected:

    inline void allocateBlock()
    {
        Block *newBlock = new Block;
        newBlock->next = blocks;
        blocks = newBlock;

        newBlock->items[blockSize - 1].blockStorageData.next = nullptr;
        if(blockSize > 1)
        {
            for(int i = 0; i < blockSize - 1; ++i)
            {
                newBlock->items[i].blockStorageData.next = &(newBlock->items[i + 1]);
            }
        }

        freeListHead = &(newBlock->items[0]);
    }

    inline void addToUsedList(T* newItem)
    {
        if(usedListHead != nullptr)
        {
            usedListHead->blockStorageData.prev = newItem;
        }
        newItem->blockStorageData.prev = nullptr;
        newItem->blockStorageData.next = usedListHead;
        usedListHead = newItem;
    }


    inline void removeFromUsedList(T *item)
    {
        if(item == usedListHead)
        {
            usedListHead = item->blockStorageData.next;
        }
        if(item->blockStorageData.prev != nullptr)
        {
            item->blockStorageData.prev->blockStorageData.next =
                item->blockStorageData.next;
        }
        if(item->blockStorageData.next != nullptr)
        {
            item->blockStorageData.next->blockStorageData.prev =
                item->blockStorageData.prev;
        }
    }


private:

    struct Block
    {
        Block *next = nullptr;
        T items[blockSize];
    };

    Block *blocks = nullptr;
    T *freeListHead = nullptr;
    T *usedListHead = nullptr;
};

#endif // BLOCK_STORAGE_H

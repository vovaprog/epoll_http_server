#ifndef LIST_STORAGE_H
#define LIST_STORAGE_H

template <typename T, int blockSize = 0x400>
class ListStorage
{
public:
    struct ServiceData
    {
        T *next = nullptr;
        T *prev = nullptr;
    };

    ListStorage() = default;

    ListStorage(const ListStorage &fa) = delete;
    ListStorage(ListStorage &&fa) = delete;
    ListStorage& operator=(const ListStorage &fa) = delete;
    ListStorage& operator=(ListStorage && fa) = delete;

    ~ListStorage()
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
            Block *newBlock = new Block;
            newBlock->next = blocks;
            blocks = newBlock;

            newBlock->items[blockSize - 1].listStorageData.next = nullptr;
            if(blockSize > 1)
            {
                for(int i = 0; i < blockSize - 1; ++i)
                {
                    newBlock->items[i].listStorageData.next = &(newBlock->items[i + 1]);
                }
            }

            freeListHead = &(newBlock->items[0]);
        }

        T *result = freeListHead;
        freeListHead = result->listStorageData.next;

        addToUsedList(result);

        return result;
    }


    inline void free(T *p)
    {
        removeFromUsedList(p);

        p->listStorageData.next = freeListHead;
        freeListHead = p;
    }


    inline T* head() const
    {
        return usedListHead;
    }

    inline T* next(const T* item) const
    {
        return item->listStorageData.next;
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
            cur = cur->listStorageData.next;
        }

        si.allocatedCount = counter;

        return 0;
    }


protected:

    inline void addToUsedList(T* newItem)
    {
        if(usedListHead != nullptr)
        {
            usedListHead->listStorageData.prev = newItem;
        }
        newItem->listStorageData.prev = nullptr;
        newItem->listStorageData.next = usedListHead;
        usedListHead = newItem;
    }


    inline void removeFromUsedList(T *item)
    {
        if(item == usedListHead)
        {
            usedListHead = item->listStorageData.next;
        }
        if(item->listStorageData.prev != nullptr)
        {
            item->listStorageData.prev->listStorageData.next =
                item->listStorageData.next;
        }
        if(item->listStorageData.next != nullptr)
        {
            item->listStorageData.next->listStorageData.prev =
                item->listStorageData.prev;
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

#endif // LIST_STORAGE_H

#ifndef FIXED_SIZE_ALLOCATOR_H
#define FIXED_SIZE_ALLOCATOR_H

#include <stdio.h>

template <typename T, int blockSize = 0x400>
class FixedSizeAllocator
{
public:
    FixedSizeAllocator() = default;

    FixedSizeAllocator(const FixedSizeAllocator &fa) = delete;
    FixedSizeAllocator(FixedSizeAllocator &&fa) = delete;
    FixedSizeAllocator& operator=(const FixedSizeAllocator &fa) = delete;
    FixedSizeAllocator& operator=(FixedSizeAllocator && fa) = delete;

    ~FixedSizeAllocator()
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
    }

    inline T* allocate()
    {
        if(freeListHead == nullptr)
        {
            //printf("allocation\n");
            Block *newBlock = new Block;
            newBlock->next = blocks;
            blocks = newBlock;

            newBlock->items[blockSize - 1].next = nullptr;
            if(blockSize > 1)
            {
                for(int i = 0; i < blockSize - 1; ++i)
                {
                    newBlock->items[i].next = &(newBlock->items[i + 1]);
                }
            }

            freeListHead = &(newBlock->items[0]);
        }

        Item *result = freeListHead;
        freeListHead = result->next;

        return new(reinterpret_cast<void*>(result)) T;
        //return reinterpret_cast<T*>(result);
    }

    inline void free(T *p)
    {
        p->~T();

        Item *item = reinterpret_cast<Item*>(p);
        item->next = freeListHead;
        freeListHead = item;
    }

private:

    union Item
    {
        Item *next = nullptr;
        char data[sizeof(T)];
    };

    struct Block
    {
        Block *next = nullptr;
        Item items[blockSize];
    };

    Block *blocks = nullptr;
    Item *freeListHead = nullptr;
};

#endif // FIXED_SIZE_ALLOCATOR_H

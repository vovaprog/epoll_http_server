#ifndef FIXED_SIZE_ALLOCATOR_H
#define FIXED_SIZE_ALLOCATOR_H

#include <stdio.h>

#include <stdlib.h>
#include <string.h>


template <typename T, int blockSize = 0x1000>
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
        for(Block *block = blocks; block != nullptr; )
        {
            Block *tempBlock = block->next;
            delete block;
            block = tempBlock;
        }
        blocks = nullptr;
        freeListHead = nullptr;
    }

    inline T* allocate()
    {
        if(freeListHead == nullptr)
        {
            Block *newBlock = new Block;
            //memset(newBlock, 0x33, sizeof(Block));
            newBlock->next = blocks;
            blocks = newBlock;

            newBlock->items[blockSize - 1].next = nullptr;
            if(blockSize > 1)
            {
                for(int i = 0; i < blockSize - 1; ++i)
                {
                    newBlock->items[i].next = &(newBlock->items[i+1]);
                }
            }

            freeListHead = &(newBlock->items[0]);
        }

        Item *result = freeListHead;
        freeListHead = result->next;

        return new (reinterpret_cast<void*>(result)) T;
        //return reinterpret_cast<T*>(result);
    }

    inline void free(T *item)
    {
        item->~T();

        Item *tempHead = freeListHead;
        freeListHead = reinterpret_cast<Item*>(item);
        freeListHead->next = tempHead;
    }

private:

    union Item {
        Item *next = nullptr;
        char data[sizeof(T)];
    };

    struct Block {
        Block *next = nullptr;
        Item items[blockSize];
    };

    Block *blocks = nullptr;
    Item *freeListHead = nullptr;
};

#endif

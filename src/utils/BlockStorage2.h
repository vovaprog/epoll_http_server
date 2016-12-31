#ifndef BLOCK_STORAGE2_H
#define BLOCK_STORAGE2_H

#include <stdio.h>

template<class T, int blockSize = 1024>
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
        printf("~~~ A\n");

        for(Block *block = blocks; block != nullptr; )
        {
            printf("~~~ B\n");

            Block *tempBlock = block->next;
            delete block;
            block = tempBlock;
        }
        blocks = nullptr;
        freeListHead = nullptr;
    }


    T* allocate()
    {
        printf("alloc\n");

        if(freeListHead == nullptr)
        {
            Block *newBlock = new Block;
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

        return reinterpret_cast<T*>(result);
    }

    int free(T *data)
    {
        printf("free\n");

        Item *tempHead = freeListHead;
        freeListHead = reinterpret_cast<Item*>(data);
        freeListHead->next = tempHead;

        return 0;
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

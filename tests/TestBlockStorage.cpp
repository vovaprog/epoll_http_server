#include <BlockStorage.h>
#include <ListStorage.h>

#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <set>
#include <list>
#include <forward_list>

struct Data
{
    int intValue1, intValue2;
    char buf[200];
    BlockStorage<Data>::ServiceData bsData;
    ListStorage<Data>::ServiceData listStorageData;
};


inline long long int getMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()
           ).count();
}

//====================================================================

class AllocateFunctor
{
public:
    AllocateFunctor(BlockStorage<Data> *storage): storage(storage) { }

    Data* operator()(int size)
    {
        return storage->allocate();
    }

private:
    BlockStorage<Data> *storage = nullptr;
};

class FreeFunctor
{
public:
    FreeFunctor(BlockStorage<Data> *storage): storage(storage) { }

    void operator()(Data *data)
    {
        storage->free(data);
    }

private:
    BlockStorage<Data> *storage = nullptr;
};

//====================================================================

void checkStorageEmpty(BlockStorage<Data> &storage)
{
    BlockStorage<Data>::StorageInfo si;
    if(storage.getStorageInfo(si) == 0)
    {
        printf("executors. blocks: %d   empty: %d   blocksize: %d   maxBlocks: %d\n",
               si.allocatedBlocks, si.emptyItems, si.blockSize, si.maxBlocks);
    }
    else
    {
        printf("getStorageInfo failed!\n");
        exit(-1);
    }

    if(si.allocatedBlocks * si.blockSize != si.emptyItems)
    {
        printf("storage is not empty!\n");
        exit(-1);
    }
}

//====================================================================

template<typename Allocate, typename Free>
void testStorage(Allocate allocFun, Free freeFun, const int totalItems, const int blockSize)
{
    std::vector<Data*> pointers;
    pointers.reserve(totalItems);

    for(int i = 0; i < totalItems; ++i)
    {
        Data *ptr = (Data*)allocFun(sizeof(Data));
        if(ptr == nullptr)
        {
            printf("allocation failed!\n");
            exit(-1);
        }
        pointers.push_back(ptr);
    }

    const int numberOfIters = 5;
    const int testBlock = 10;

    for(int q = 0; q < numberOfIters; ++q)
    {
        for(int start = 0; start < testBlock; ++start)
        {
            for(int ind = start; ind < totalItems; ind += testBlock)
            {
                freeFun(pointers[ind]);
            }
            for(int ind = start; ind < totalItems; ind += testBlock)
            {
                Data *ptr = (Data*)allocFun(sizeof(Data));
                if(ptr == nullptr)
                {
                    printf("allocation failed!\n");
                    exit(-1);
                }
                pointers[ind] = ptr;
            }
        }
    }

    for(Data * p : pointers)
    {
        freeFun(p);
    }
}


void testWithMalloc()
{
    const int totalItems = 1000000;
    const int blockSize = 100;

    BlockStorage<Data> storage;
    std::vector<Data*> pointers;

    storage.init(totalItems / blockSize, blockSize);

    long long int millis;

    {
        millis = getMilliseconds();

        testStorage(AllocateFunctor(&storage), FreeFunctor(&storage), totalItems, blockSize);

        millis = getMilliseconds() - millis;
        printf("block storage: %lld\n", millis);
    }

    {
        millis = getMilliseconds();

        testStorage(malloc, free, totalItems, blockSize);

        millis = getMilliseconds() - millis;
        printf("malloc: %lld\n", millis);
    }

    checkStorageEmpty(storage);
}

//====================================================================

void testIteration()
{
    const int totalItems = 100000;
    const int blockSize = 10;
    const int testItems = 30;

    BlockStorage<Data> storage;
    std::vector<Data*> pointers;

    storage.init(totalItems / blockSize, blockSize);
    pointers.reserve(totalItems);

    for(int i = 0; i < testItems; ++i)
    {
        Data *ptr = storage.allocate();
        if(ptr == nullptr)
        {
            printf("allocation failed!\n");
            exit(-1);
        }
        ptr->intValue1 = i;
        pointers.push_back(ptr);
    }

    BlockStorage<Data>::Iterator iter = storage.begin();
    BlockStorage<Data>::Iterator end = storage.end();
    int counter = 0;

    for(; iter != end; ++iter)
    {
        if(iter->intValue1 != counter)
        {
            printf("%d != %d\n", iter->intValue1, counter);
            exit(-1);
        }
        ++counter;
    }

    std::set<int> storageValues;

    for(int i = 0; i < testItems; ++i)
    {
        if(i % 3 != 0 || (i >= 15 && i <= 20))
        {
            storage.free(pointers[i]);
            pointers[i] = nullptr;
        }
        else
        {
            storageValues.insert(pointers[i]->intValue1);
        }
    }

    for(iter = storage.begin(); iter != end; ++iter)
    {
        if(storageValues.count(iter->intValue1) != 1)
        {
            printf("value %d not found!\n", iter->intValue1);
            exit(-1);
        }
        else
        {
            storageValues.erase(iter->intValue1);
            printf("value: %d\n", iter->intValue1);
        }
    }

    printf("-----\n");

    for(auto & data : storage)
    {
        printf("value: %d\n", data.intValue1);
    }


    for(Data * p : pointers)
    {
        if(p != nullptr)
        {
            storage.free(p);
        }
    }

    checkStorageEmpty(storage);
}

//====================================================================

/*template<typename List, int ITEM_COUNT=100, int ITERATIONS=1>
void testList(const char *testName)
{
    long long int millis;

    millis = getMilliseconds();

    {
        List list;

        for(int i=0;i<ITERATIONS;++i)
        {
            for(int i=0;i<ITEM_COUNT;++i)
            {
                list.push_front(i);
            }
            for(int i=0;i<ITEM_COUNT;++i)
            {
                list.pop_front();
            }
        }
    }

    millis = getMilliseconds() - millis;
    printf("%s: %lld\n", testName, millis);
}

void testLists()
{
    testList<std::forward_list<int, FixedSizeAllocator<int>>>("forward_list fixed size allocator");
    testList<std::forward_list<int>>("forward_list default allocator");

    testList<std::list<int, FixedSizeAllocator<int>>>("list fixed size allocator");
    testList<std::list<int>>("list default allocator");
}*/

void testListStorage()
{
    ListStorage<Data> ls;

    for(int i = 1;i<=10;++i)
    {
        Data *data = ls.allocate();
        data->intValue1 = i;
    }

    for(Data *cur = ls.head();cur != nullptr;cur=ls.next(cur))
    {
        printf("item: %d\n", cur->intValue1);
    }
}

int main()
{
    //testWithMalloc();
    //testIteration();
    //testLists();
    testListStorage();

    printf("\n============\nall tests ok\n");

    return 0;
}


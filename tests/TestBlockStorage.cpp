#include <BlockStorage.h>

#include <stdlib.h>
#include <stdio.h>
#include <chrono>

struct Data {
    int intValue1, intValue2;
    char buf[200];
    BlockStorage<Data>::ServiceData bsData;
};


inline long long int getMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
        ).count();
}

//====================================================================

class AllocateFunctor {
public:
    AllocateFunctor(BlockStorage<Data> *storage): storage(storage) { }

    Data* operator()(int size)
    {
        return storage->allocate();
    }

private:
    BlockStorage<Data> *storage = nullptr;
};

class FreeFunctor {
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

    const int numberOfIters = 1;
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

    for(Data *p : pointers)
    {
        freeFun(p);
    }
}

void testStorage1()
{
    const int totalItems = 10000000;
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
}

//====================================================================

void testStorage2()
{
    BlockStorage<Data> storage2;
    std::vector<Data*> pointers;  

    storage2.init(1000, 10);
    pointers.reserve(1000 * 10);

    for(int i = 0; i < 30; ++i)
    {
        Data *ptr = storage2.allocate();
        if(ptr == nullptr)
        {
            printf("allocation failed!\n");
            exit(-1);
        }
        ptr->intValue1 = i;
        pointers.push_back(ptr);
    }

    for(int i = 0; i < 30; ++i)
    {
        if(i % 3 != 0 || (i>=15 && i<=20))
        {
            storage2.free(pointers[i]);
            pointers[i] = nullptr;
        }
    }

    BlockStorage<Data>::Iterator iter = storage2.begin();
    BlockStorage<Data>::Iterator end = storage2.end();


    for(; iter != end; ++iter)
    {
        printf("value: %d\n", iter->intValue1);
    }

    printf("-----\n");

    for(auto &data : storage2)
    {
        printf("value: %d\n", data.intValue1);
    }


    for(Data *p : pointers)
    {
        if(p != nullptr)
        {
            storage2.free(p);
        }
    }
}

//====================================================================

int main()
{
    testStorage1();
    testStorage2();

    return 0;
}


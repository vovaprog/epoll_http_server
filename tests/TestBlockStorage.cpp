#include <BlockStorage.h>

#include <stdlib.h>
#include <stdio.h>
#include <chrono>

struct Data {
    int intValue1, intValue2;
    char buf[200];
    BlockStorage<Data>::ServiceData bsData;
};

const int totalItems = 10000000;
const int blockSize = 100;

BlockStorage<Data> storage;
std::vector<Data*> pointers;


inline long long int getMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
        ).count();
}


Data* blockStorageAllocate(int size)
{
    return storage.allocate();
}

void blockStorageFree(Data *data)
{
    storage.free(data);
}

template<typename Allocate, typename Free>
void testStorage(Allocate allocFun, Free freeFun)
{
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


int main()
{
    storage.init(totalItems / blockSize, blockSize);
    pointers.reserve(totalItems);

    long long int millis;

     {
        millis = getMilliseconds();

        testStorage(blockStorageAllocate, blockStorageFree);

        millis = getMilliseconds() - millis;
        printf("block storage: %lld\n", millis);
    }
  
    pointers.erase(pointers.begin(), pointers.end());
 
    {
        millis = getMilliseconds();

        testStorage(malloc, free);

        millis = getMilliseconds() - millis;
        printf("malloc: %lld\n", millis);
   }

    return 0;
}


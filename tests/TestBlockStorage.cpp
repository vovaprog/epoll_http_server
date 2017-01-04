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
    Data()
    {
        ++constructorCallCounter;
        //printf("Data default ctr\n");
    }

    ~Data()
    {
        ++destructorCallCounter;
        //printf("Data ~~~\n");
    }

    int intValue1, intValue2;
    char buf[200];
    BlockStorage<Data>::ServiceData bsData;
    ListStorage<Data>::ServiceData listStorageData;

    static int constructorCallCounter;
    static int destructorCallCounter;
};

int Data::constructorCallCounter = 0;
int Data::destructorCallCounter = 0;

inline long long int getMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()
           ).count();
}

//====================================================================

template<typename Storage>
class AllocateFunctor
{
public:
    AllocateFunctor(Storage *storage): storage(storage) { }

    Data* operator()(int size)
    {
        return storage->allocate();
    }

private:
    Storage *storage = nullptr;
};

template<typename Storage>
class FreeFunctor
{
public:
    FreeFunctor(Storage *storage): storage(storage) { }

    void operator()(Data *data)
    {
        storage->free(data);
    }

private:
    Storage *storage = nullptr;
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
void testStorage(Allocate allocFun, Free freeFun, const int totalItems)
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

    BlockStorage<Data> blockStorage;
    ListStorage<Data> listStorage;
    std::vector<Data*> pointers;

    blockStorage.init(totalItems / blockSize, blockSize);

    long long int millis;

    {
        millis = getMilliseconds();

        testStorage(AllocateFunctor<BlockStorage<Data>>(&blockStorage),
                    FreeFunctor<BlockStorage<Data>>(&blockStorage),
                    totalItems);

        millis = getMilliseconds() - millis;
        printf("block storage: %lld\n", millis);
    }

    {
        millis = getMilliseconds();

        testStorage(AllocateFunctor<ListStorage<Data>>(&listStorage),
                    FreeFunctor<ListStorage<Data>>(&listStorage),
                    totalItems);

        millis = getMilliseconds() - millis;
        printf("list storage: %lld\n", millis);
    }

    {
        millis = getMilliseconds();

        testStorage(malloc, free, totalItems);

        millis = getMilliseconds() - millis;
        printf("malloc: %lld\n", millis);
    }

    checkStorageEmpty(blockStorage);
    if(listStorage.head() != nullptr)
    {
        printf("list storage is not empty!\n");
        exit(-1);
    }
}

//====================================================================

void printListStorage(ListStorage<Data> &ls)
{
    printf("=======\n");
    for(Data *cur = ls.head(); cur != nullptr; cur = ls.next(cur))
    {
        printf("%d ", cur->intValue1);
    }
    printf("\n=======\n");
}


void checkListStorage(ListStorage<Data> &ls, std::set<int> vals)
{
    std::set<int>::size_type counter = 0;
    for(Data *cur = ls.head(); cur != nullptr; cur = ls.next(cur))
    {
        ++counter;
        if(vals.count(cur->intValue1) != 1)
        {
            printf("invalid value!\n");
            exit(-1);
        }
    }
    if(counter != vals.size())
    {
        printf("invalid number of items!\n");
        exit(-1);
    }
}


void testListStorage()
{
    const int ITEM_COUNT = 100;

    ListStorage<Data> ls;
    std::set<int> vals;

    Data *pointers[ITEM_COUNT];

    int startCounter = Data::constructorCallCounter;

    for(int i = 0; i < ITEM_COUNT; ++i)
    {
        Data *data = ls.allocate();
        data->intValue1 = i;
        vals.insert(i);
        pointers[i] = data;
    }

    if(Data::constructorCallCounter - startCounter != ITEM_COUNT)
    {
        printf("Invalid number of constructor calls\n");
        exit(-1);
    }

    printListStorage(ls);
    checkListStorage(ls, vals);

    startCounter = Data::destructorCallCounter;

    for(int i = 0; i < ITEM_COUNT; i += 2)
    {
        vals.erase(pointers[i]->intValue1);
        ls.free(pointers[i]);
    }

    if(Data::destructorCallCounter - startCounter != ITEM_COUNT / 2)
    {
        printf("Invalid number of destructor calls\n");
        exit(-1);
    }

    printListStorage(ls);
    checkListStorage(ls, vals);

    for(int i = 1; i < ITEM_COUNT; i += 2)
    {
        vals.erase(pointers[i]->intValue1);
        ls.free(pointers[i]);
    }

    printListStorage(ls);
    checkListStorage(ls, vals);

    for(int i = 0; i < ITEM_COUNT; ++i)
    {
        Data *data = ls.allocate();
        data->intValue1 = i;
        vals.insert(i);
        pointers[i] = data;
    }

    printListStorage(ls);
    checkListStorage(ls, vals);

    for(int i = 0; i < ITEM_COUNT; ++i)
    {
        vals.erase(pointers[i]->intValue1);
        ls.free(pointers[i]);
    }

    printListStorage(ls);
    checkListStorage(ls, vals);

    if(ls.head() != nullptr)
    {
        printf("ListStorage is not empty!\n");
        exit(-1);
    }
}

int main()
{
    testWithMalloc();
    testListStorage();

    printf("\n============\nall tests ok\n");

    return 0;
}


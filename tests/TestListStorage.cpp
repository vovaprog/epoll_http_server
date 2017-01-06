#include <ListStorage.h>

#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <set>
#include <vector>

struct Data
{
    Data()
    {
        ++constructorCallCounter;
    }

    ~Data()
    {
        ++destructorCallCounter;
    }

    int intValue1, intValue2;
    char buf[200];
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

    ListStorage<Data> listStorage;
    std::vector<Data*> pointers;

    long long int millis;

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

    if(listStorage.head() != nullptr)
    {
        printf("list storage is not empty!\n");
        exit(-1);
    }
}

//====================================================================

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

template<bool CheckConstructor>
class CallChecker {
public:
    CallChecker(int expectedValue):
        expectedValue(expectedValue)
    {
        startCounter = CheckConstructor ?
            Data::constructorCallCounter : Data::destructorCallCounter;
    }
    ~CallChecker()
    {
        int curCounter = CheckConstructor ?
            Data::constructorCallCounter : Data::destructorCallCounter;

        if(curCounter - startCounter != expectedValue)
        {
            printf("invalid number of constructor/destructor calls!\n");
            exit(-1);
        }
    }
private:
    int startCounter;
    int expectedValue;
};

void testListStorage()
{
    const int ITEM_COUNT = 10000;

    ListStorage<Data> ls;
    std::set<int> vals;

    Data *pointers[ITEM_COUNT];

    {
        CallChecker<true> callChecker(ITEM_COUNT);

        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            Data *data = ls.allocate();
            data->intValue1 = i;
            vals.insert(i);
            pointers[i] = data;
        }

        checkListStorage(ls, vals);
    }    

    {
        CallChecker<false> checker(ITEM_COUNT / 2);

        for(int i = 0; i < ITEM_COUNT; i += 2)
        {
            vals.erase(pointers[i]->intValue1);
            ls.free(pointers[i]);
        }

        checkListStorage(ls, vals);
    }

    {
        CallChecker<false> checker(ITEM_COUNT / 2);

        for(int i = 1; i < ITEM_COUNT; i += 2)
        {
            vals.erase(pointers[i]->intValue1);
            ls.free(pointers[i]);
        }

        checkListStorage(ls, vals);
    }

    {
        CallChecker<true> checker(ITEM_COUNT);

        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            Data *data = ls.allocate();
            data->intValue1 = i;
            vals.insert(i);
            pointers[i] = data;
        }

        checkListStorage(ls, vals);
    }

    {
        CallChecker<false> checker(ITEM_COUNT);

        for(int i = 0; i < ITEM_COUNT; ++i)
        {
            vals.erase(pointers[i]->intValue1);
            ls.free(pointers[i]);
        }

        checkListStorage(ls, vals);
    }

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


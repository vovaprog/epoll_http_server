#ifndef LIST_STORAGE_H
#define LIST_STORAGE_H

#include <FixedSizeAllocator.h>

template<typename T, typename Allocator = FixedSizeAllocator<T>>
class ListStorage
{
public:
    struct ServiceData
    {
        T *next = nullptr;
        T *prev = nullptr;
    };

    ListStorage() = default;

    ListStorage(const ListStorage &ls) = delete;
    ListStorage(ListStorage &&ls) = delete;
    ListStorage& operator=(const ListStorage &ls) = delete;
    ListStorage& operator=(ListStorage && ls) = delete;

    ~ListStorage()
    {
        T *cur = _head;
        while(cur != nullptr)
        {
            T *temp = cur->listStorageData.next;
            allocator.free(cur);
            cur = temp;
        }
    }


    inline T* allocate()
    {
        T* newItem = allocator.allocate();
        //printf("%llx\n",(long long int)newItem->listStorageData.prev);

        if(_head != nullptr)
        {
            _head->listStorageData.prev = newItem;
        }
        newItem->listStorageData.next = _head;
        _head = newItem;
        return _head;
    }

    void free(T *item)
    {
        if(item == _head)
        {
            _head = item->listStorageData.next;
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

        allocator.free(item);
    }

    inline T* head()
    {
        return _head;
    }

    inline T* next(const T* item)
    {
        return item->listStorageData.next;
    }

protected:

    Allocator allocator;
    T *_head = nullptr;
};

#endif // LIST_STORAGE_H

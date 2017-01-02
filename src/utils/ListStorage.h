#ifndef LIST_STORAGE_H
#define LIST_STORAGE_H

#include <FixedSizeAllocator.h>

template<typename T, typename Allocator = FixedSizeAllocator<T>>
class ListStorage {
public:
    struct ServiceData {
        T *next = nullptr;
        T *prev = nullptr;
    };

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


    T* allocate()
    {
        T* newItem = allocator.allocate();
        newItem->listStorageData.prev = _tail;
        if(_tail != nullptr)
        {
            _tail->listStorageData.next = newItem;
        }
        _tail = newItem;
        if(_head == nullptr)
        {
            _head = newItem;
        }
        return _tail;
    }

    void free(T *item)
    {
        if(item->listStorageData.prev)
        {
            item->listStorageData.prev->listStorageData.next =
                    item->listStorageData->next;
        }
        if(item->listStorageData.next)
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
    T *_tail = nullptr;
};

#endif // LIST_STORAGE_H

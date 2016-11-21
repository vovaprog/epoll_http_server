#include <BlockStorage.h>

struct Data {
    int index;
    char buf[200];
};

BlockStorage<Data> storage;

int main()
{
    const int totalItems = 1000000;
    const int blockSize = 100;
    storage.init(totalItems / blockSize, blockSize);
    std::vector<Data*> pointers;
    pointers.reserve(totalItems);

    for(int i = 0; i < totalItems; ++i)
    {
        pointers.push_back(storage.allocate());
    }

    return 0;
}


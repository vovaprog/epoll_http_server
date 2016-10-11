#ifndef TRANSFER_RING_BUFFER_H
#define TRANSFER_RING_BUFFER_H

class TransferRingBuffer
{
public:
    TransferRingBuffer() = default;

    TransferRingBuffer(int size): bufSize(size)
    {
        buf = new char[size];
    }

    ~TransferRingBuffer()
    {
        if(buf != nullptr)
        {
            delete[] buf;
        }
    }

    TransferRingBuffer(const TransferRingBuffer &tm) = delete;
    TransferRingBuffer(TransferRingBuffer &&tm) = delete;
    TransferRingBuffer& operator=(const TransferRingBuffer &tm) = delete;
    TransferRingBuffer& operator=(TransferRingBuffer && tm) = delete;

    void init(int newSize)
    {
        if(newSize != bufSize)
        {
            if(buf != nullptr)
            {
                delete[] buf;
                buf = nullptr;
            }
            bufSize = newSize;
            buf = new char[bufSize];
        }
        writeHead = 0;
        readHead = 0;
    }

    void clear()
    {
        writeHead = 0;
        readHead = 0;
    }

    bool startWrite(void* &data, int &size)
    {
        if(writeHead == bufSize)
        {
            if(readHead == 0)
            {
                return false;
            }
            else
            {
                writeHead = 0;
            }
        }

        data = buf + writeHead;

        if(writeHead >= readHead)
        {
            size = bufSize - writeHead;
        }
        else
        {
            size = readHead - writeHead - 1;
        }

        return (size > 0);
    }

    void endWrite(int size)
    {
        writeHead += size;
    }

    bool startRead(void* &data, int &size)
    {
        data = buf + readHead;

        if(readHead < writeHead)
        {
            size = writeHead - readHead;
        }
        else if(readHead > writeHead)
        {
            size = bufSize - readHead;
        }
        else
        {
            size = 0;
        }

        return (size > 0);
    }

    void endRead(int size)
    {
        readHead += size;
        if(readHead == bufSize)
        {
            readHead = 0;
            if(writeHead == bufSize)
            {
                writeHead = 0;
            }
        }
    }

    bool readAvailable()
    {
        void *data;
        int size;
        return startRead(data, size);
    }

#ifdef TRANSFER_RING_BUFFER_DEBUG
    void printInfo()
    {
        printf("writeHead: %d   readHead: %d\n", writeHead, readHead);
    }
#endif

protected:
    char *buf = nullptr;
    int readHead = 0, writeHead = 0, bufSize = 0;
};

#endif // TRANSFER_RING_BUFFER_H

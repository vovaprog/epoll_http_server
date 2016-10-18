#include <ctime>
#include <chrono>

long long int getMilliseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch()
           ).count();
}

int getCurrentTimeString(char *timeBuffer, int timeBufferSize)
{
    time_t t;
    struct tm timeinfo;

    t = time(nullptr);
    localtime_r(&t, &timeinfo);
    if(strftime(timeBuffer, timeBufferSize, "%Y%m%d %H:%M:%S", &timeinfo) > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}


#include <HttpUtils.h>

#include <unordered_map>

static std::unordered_map<int, const char*> c2s;

void initHttpUtils()
{
    c2s[HttpCode::ok] = "OK";
    c2s[HttpCode::notModified] = "Not Modified";
    c2s[HttpCode::notFound] = "Not Found";
}


const char* httpCode2String(int code)
{
    auto iter = c2s.find(code);
    if(iter == c2s.end())
    {
        return nullptr;
    }
    return iter->second;
}


static int hex2int(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }
    c = (c | 0x20); // to lower case
    if(c >= 'a' && c<='f')
    {
        return c - 'a' + 0xA;
    }
    return -1;
}


int percentDecode(const char *src, char *dst, int srcLength)
{
    int si = 0;
    int di = 0;

    while(si < srcLength && src[si] != 0)
    {
        if(src[si] == '%' && si + 2 < srcLength)
        {
            int v1 = hex2int(src[si+1]);
            if(v1 < 0)
            {
                return -1;
            }
            int v2 = hex2int(src[si+2]);
            if(v2 < 0)
            {
                return -1;
            }
            dst[di] = ((v1 << 4) | v2);

            si += 3;
            ++di;
        }
        else
        {
            dst[di] = src[si];
            ++si;
            ++di;
        }
    }

    dst[di] = 0;

    return 0;
}

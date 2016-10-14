#include <HttpUtils.h>

#include <unordered_map>

std::unordered_map<int, const char*> c2s;

void initHttpUtils()
{
    c2s[HttpCode::ok] = "OK";
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


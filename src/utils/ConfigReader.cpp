#include <ConfigReader.h>

#include <fstream>
#include <sstream>

namespace
{

bool isSpace(char c)
{
    return c == ' ' || c == '\t';
}

bool isBreak(char c)
{
    return c == '\r' || c == '\n';
}

bool isKey(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c < '9') || c == '.';
}

const char CommentSymbol = '#';
const char ParamSeparatorSymbol = '=';

}

bool configReadString(const char *s, ConfigMapType &params)
{
    int i = 0;

    while (true)
    {
        //===== all spaces and empty lines before key =====

        for (; s[i] != 0 && (isSpace(s[i]) || isBreak(s[i])); ++i);

        if (s[i] == 0) return true;

        if (s[i] == CommentSymbol)
        {
            //===== read comment till end of line =====

            for (; s[i] != 0 && !isBreak(s[i]); ++i);
        }
        else if (isKey(s[i]))
        {
            //===== read key =====

            int keyStart = i;

            for (; s[i] != 0 && isKey(s[i]); ++i);
            if (!isSpace(s[i]) && s[i] != ParamSeparatorSymbol)
            {
                return false;
            }

            int keyEnd = i;


            //===== pass spaces after key =====

            for (; s[i] != 0 && isSpace(s[i]); ++i);

            //===== pass param separator =====

            if (s[i] != ParamSeparatorSymbol)
            {
                return false;
            }
            ++i;

            //===== pass spaces after param separator =====

            for (; s[i] != 0 && isSpace(s[i]); ++i);
            if (s[i] == 0)
            {
                return false;
            }


            //===== read value =====

            int valueStart = i;

            for (; s[i] != 0 && !isBreak(s[i]); ++i);

            int valueEnd = i;


            //===== remove trailing spaces =====

            for (; valueEnd > valueStart && isSpace(s[valueEnd]); --valueEnd);

            if (valueEnd <= valueStart)
            {
                return false;
            }


            //===== add parameter =====

            params[std::string(s + keyStart, keyEnd - keyStart)] = std::string(s + valueStart, valueEnd - valueStart);
        }
        else
        {
            return false;
        }
    }

    return true;
}


bool configReadFile(const char *fileName, ConfigMapType &params)
{
    std::ifstream fs(fileName);

    if (fs.fail())
    {
        return false;
    }

    std::string line;

    while (std::getline(fs, line))
    {
        if (!configReadString(line.c_str(), params))
        {
            return false;
        }
    }

    return true;
}


std::string configToString(const std::map<std::string, std::string> &params)
{
    std::stringstream s;

    for (auto &p : params)
    {
        s << p.first << ParamSeparatorSymbol << p.second << std::endl;
    }

    return s.str();
}


#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <map>
#include <string>

typedef std::map<std::string, std::string> ConfigMapType;

// read multiline config string with comments.
bool configReadString(const char *s, ConfigMapType &params);

bool configReadFile(const char *fileName, ConfigMapType &params);

std::string configToString(const ConfigMapType &params);

#endif


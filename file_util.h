#ifndef FILE_UTIL_H
#define FILE_UTIL_H
#include <stdint.h>
#include <vector>
#include <string>

bool read_file(std::string filename, std::vector<uint8_t> &content);
#endif
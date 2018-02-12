#include "file_util.h"
#include <stdio.h>


bool read_file(std::string filename, std::vector<uint8_t> &content)
{
  FILE* file_in = NULL;

  file_in = fopen(filename.c_str(),"rb");

  if(!file_in)
    return false;

  size_t file_size = 0ULL;
  fseek(file_in,0,SEEK_END);
  file_size = ftell(file_in);
  fseek(file_in,0,SEEK_SET);
  content.resize(file_size);
  fread(&content[0],1,file_size,file_in);
  fclose(file_in);
	return true;
}
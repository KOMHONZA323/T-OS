#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>

void init_fat16();
int fat16_read_file(const char* filename, void* buffer);
int fat16_create_file(const char* filename);
void fat16_list_directory();

#endif

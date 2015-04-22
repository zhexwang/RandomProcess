#ifndef _TYPE_H_
#define _TYPE_H_

#include <stdbool.h>
#include <stdlib.h>

typedef bool BOOL;
typedef char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;
typedef long long INT64;
typedef unsigned long long UINT64;
typedef unsigned long ADDR;
typedef unsigned long ORIGIN_ADDR;
typedef unsigned long CODE_CACHE_ADDR;
typedef size_t SIZE;
typedef size_t ORIGIN_SIZE;
typedef size_t CODE_CACHE_SIZE;

#define THREAD_MAX_NUM 128

#endif

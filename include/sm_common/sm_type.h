#include <stdio.h>
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef void* HANDLE;

typedef struct sm_fraction {
    int32_t num;
    int32_t den;
}sm_fraction_t;
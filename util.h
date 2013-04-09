#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <stdint.h>

static inline uintptr_t idivc(uintptr_t x, uintptr_t y)
{
    return (x ? (((x-1)/y) + 1) : 0);
}

#endif

#ifndef _UTIL_H_
#define _UTIL_H_ 1

#include <stdint.h>

extern void idivc(uint32_t x, uint32_t y);
#define idivc(x, y) (x ? (((x-1)/y) + 1) : 0)

#endif

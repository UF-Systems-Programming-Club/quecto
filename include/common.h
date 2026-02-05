#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>

#define array_append(array, item)\
    do {\
        if (array.count >= array.capacity) {\
            if (array.capacity == 0) array.capacity = 128;\
            array.capacity *= 256;\
            array.items = realloc(array.items, array.capacity * sizeof(*array.items));\
        }\
        array.items[array.count++] = item;\
    } while (0)

#define array_pop(array)\
    do {\
        if (array.count != 0) array.count--;\
    } while(0)

#endif
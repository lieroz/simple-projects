#include "binsearch.h"

int binsearch(void *array, int size, int type_size, void *value, int (*compare)(void *, void *))
{
    char *begin = array;
    char *end = begin + size * type_size;

    while (begin <= end)
    {
        char *mid = begin + (((end - begin) / (type_size * 2))) * type_size;

        if (compare(mid, value) == 0)
        {
            return (int)((mid - (char *)array) / type_size);
        }

        if (compare(mid, value) < 0)
        {
            begin = mid + type_size;
        }
        else
        {
            end = mid - type_size;
        }
    }

    return -1;
}

int binsearch_recursive(void *array, void *iter, int size, int type_size, void *value, int (*compare)(void *, void *))
{
    char *begin = iter;
    char *end = begin + size * type_size;

    if (begin <= end && size > 0)
    {
        char *mid = begin + (((end - begin) / (type_size * 2))) * type_size;

        if (compare(mid, value) == 0)
        {
            return (int)((mid - (char *)array) / type_size);
        }

        if (compare(mid, value) < 0)
        {
            return binsearch_recursive(array, mid + type_size, (end - mid) / type_size - 1, type_size, value, compare);
        }

        return binsearch_recursive(array, begin, (mid - (char *)iter) / type_size, type_size, value, compare);
    }

    return -1;
}

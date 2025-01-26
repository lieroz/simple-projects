#include "binsearch.h"

int binsearch(void *array, int size, int type_size, void *value, int (*compare)(void *, void *))
{
    char *begin = array;
    char *end = begin + size * type_size;

    /*
     * <= in a cycle comparison is used in a case
     * when on last iteration begin became equal to end
     * but the answer is still not returned
     */
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

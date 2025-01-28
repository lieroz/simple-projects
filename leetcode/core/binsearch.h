int binsearch(void *array, int size, int type_size, void *value, int (*compare)(void *, void *));
int binsearch_recursive(void *array, void *iter, int size, int type_size, void *value, int (*compare)(void *, void *));

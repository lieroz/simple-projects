#include "../core/sliding_window.h"

#include <assert.h>
#include <stdio.h>

void test1()
{
    int array[] = {100, 200, 300, 400};
    int result = find_max_sum_of_all_subarrays_of_size_k(array, sizeof(array) / sizeof(array[0]), 2);
    assert(result == 700);
}

void test2()
{
    int array[] = {1, 4, 2, 10, 23, 3, 1, 0, 20};
    int result = find_max_sum_of_all_subarrays_of_size_k(array, sizeof(array) / sizeof(array[0]), 4);
    assert(result == 39);
}

void test3()
{
    int array[] = {2, 3};
    int result = find_max_sum_of_all_subarrays_of_size_k(array, sizeof(array) / sizeof(array[0]), 3);
    assert(result == -1);
}

int main(int argc, char *argv[])
{
    int default_choice = 1;
    int choice = default_choice;

    if (argc > 1)
    {
        if (sscanf_s(argv[1], "%d", &choice) != 1)
        {
            printf("Couldn't parse input as a number.\n");
            return -1;
        }
    }

    switch (choice)
    {
    case 1:
        test1();
        break;
    case 2:
        test2();
        break;
    case 3:
        test3();
        break;
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
    }

    return 0;
}

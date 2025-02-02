#include "../core/sliding_window.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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

void test4()
{
    int array[] = {1, 4, 45, 6, 10, 19};
    int result = find_smallest_subarray_with_sum_greater_that_value(array, sizeof(array) / sizeof(array[0]), 51);
    assert(result == 3);
}

void test5()
{
    int array[] = {1, 52};
    int result = find_smallest_subarray_with_sum_greater_that_value(array, sizeof(array) / sizeof(array[0]), 51);
    assert(result == 1);
}

void test6()
{
    int array[] = {};
    int result = find_smallest_subarray_with_sum_greater_that_value(array, 0, 51);
    assert(result == 0);
}

void test7()
{
    int array[] = {0, 1, 3, 5, 7, 9};
    int result = find_smallest_subarray_with_sum_greater_that_value(array, sizeof(array) / sizeof(array[0]), 51);
    assert(result == 0);
}

void test8()
{
	int array[] = {15, 2, 4, 8, 9, 5, 10, 23};
	int indexes[2];
	find_subarray_with_given_sum_in_an_array_of_non_negative_integers(array, sizeof(array) / sizeof(array[0]), 23, indexes);
	assert(indexes[0] == 1 && indexes[1] == 5);
}

void test9()
{
	int array[] = {1, 10, 4, 0, 3, 5};
	int indexes[2];
	find_subarray_with_given_sum_in_an_array_of_non_negative_integers(array, sizeof(array) / sizeof(array[0]), 7, indexes);
	assert(indexes[0] == 2 && indexes[1] == 5);
}

void test10()
{
	int array[] = {1, 4};
	int indexes[2];
	find_subarray_with_given_sum_in_an_array_of_non_negative_integers(array, sizeof(array) / sizeof(array[0]), 0, indexes);
	assert(indexes[0] == -1 && indexes[1] == -1);
}

void test11()
{
	char* str = "aabcbcdbca";
	int indexes[2];
	find_smallest_window_that_contains_all_characters_of_string_itself(str, strlen(str), indexes);
	assert(indexes[0] == 6 && indexes[1] == 10);
}

void test12()
{
	char* str = "aaab";
	int indexes[2];
	find_smallest_window_that_contains_all_characters_of_string_itself(str, strlen(str), indexes);
	assert(indexes[0] == 2 && indexes[1] == 4);
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
    case 4:
        test4();
        break;
    case 5:
        test5();
        break;
    case 6:
        test6();
        break;
    case 7:
        test7();
        break;
    case 8:
        test8();
        break;
    case 9:
        test9();
        break;
    case 10:
        test10();
        break;
    case 11:
        test11();
        break;
    case 12:
        test11();
        break;
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
    }

    return 0;
}

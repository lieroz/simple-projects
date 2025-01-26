#include "../core/binsearch.h"

#include <assert.h>
#include <stdio.h>

int i_compare(void *lhs, void *rhs)
{
    int i_lhs = *(int *)lhs;
    int i_rhs = *(int *)rhs;
    return i_lhs - i_rhs;
}

void test_empty_array()
{
    int array[] = {};
    int value = 3;
    int result = binsearch(array, 0, sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_array_of_size_1_value_found()
{
    int array[] = {1};
    int value = 1;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == 0);
}

void test_array_of_size_1_lesser_value_not_found()
{
    int array[] = {1};
    int value = 0;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_array_of_size_1_greater_value_not_found()
{
    int array[] = {1};
    int value = 2;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_array_of_size_2_value_found()
{
    int array[] = {1, 2};
    int value = 1;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == 0);
}

void test_array_of_size_2_lesser_value_not_found()
{
    int array[] = {1, 2};
    int value = 0;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_array_of_size_2_greater_value_not_found()
{
    int array[] = {1, 2};
    int value = 3;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_value_found_array_size_even()
{
    int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int value = 3;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == 2);
}

void test_value_found_array_size_odd()
{
    int array[] = {1, 2, 3, 4, 5, 6, 7};
    int value = 3;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == 2);
}

void test_lesser_value_not_found_array_size_even()
{
    int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int value = 0;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_lesser_value_not_found_array_size_odd()
{
    int array[] = {1, 2, 3, 4, 5, 6, 7};
    int value = 0;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_greater_value_not_found_array_size_even()
{
    int array[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int value = 9;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
    assert(result == -1);
}

void test_greater_value_not_found_array_size_odd()
{
    int array[] = {1, 2, 3, 4, 5, 6, 7};
    int value = 8;
    int result = binsearch(array, sizeof(array) / sizeof(int), sizeof(int), &value, &i_compare);
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
        test_empty_array();
        break;
    case 2:
        test_array_of_size_1_value_found();
        break;
    case 3:
        test_array_of_size_1_lesser_value_not_found();
        break;
    case 4:
        test_array_of_size_1_greater_value_not_found();
        break;
    case 5:
        test_array_of_size_2_value_found();
        break;
    case 6:
        test_array_of_size_2_lesser_value_not_found();
        break;
    case 7:
        test_array_of_size_2_greater_value_not_found();
        break;
    case 8:
        test_value_found_array_size_even();
        break;
    case 9:
        test_value_found_array_size_odd();
        break;
    case 10:
        test_lesser_value_not_found_array_size_even();
        break;
    case 11:
        test_lesser_value_not_found_array_size_odd();
        break;
    case 12:
        test_greater_value_not_found_array_size_even();
        break;
    case 13:
        test_greater_value_not_found_array_size_odd();
        break;
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
    }

    return 0;
}

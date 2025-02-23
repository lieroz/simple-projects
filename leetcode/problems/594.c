#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

int findLHS(int *nums, int numsSize)
{
    qsort(nums, numsSize, sizeof(int), compare);

    int res = 0, l = 0, r = 1;
    while (r < numsSize)
    {
        int diff = nums[r] - nums[l];
        if (diff == 1)
        {
            res = fmax(res, r - l + 1);
            r++;
        }
        else if (diff < 1)
        {
            r++;
        }
        else
        {
            l++;
        }
    }

    return res;
}

void test1()
{
    int nums[] = {1, 3, 2, 2, 5, 2, 3, 7};
    assert(findLHS(nums, sizeof(nums) / sizeof(int)) == 5);
}

void test2()
{
    int nums[] = {1, 2, 3, 4};
    assert(findLHS(nums, sizeof(nums) / sizeof(int)) == 2);
}

void test3()
{
    int nums[] = {1, 1, 1, 1};
    assert(findLHS(nums, sizeof(nums) / sizeof(int)) == 0);
}

void test4()
{
    int nums[] = {1, 2, 2, 1};
    assert(findLHS(nums, sizeof(nums) / sizeof(int)) == 4);
}

void test5()
{
    int nums[] = {-3, -1, -1, -1, -3, -2};
    assert(findLHS(nums, sizeof(nums) / sizeof(int)) == 4);
}

void test6()
{
    int nums[] = {1, 2, 1, 3, 0, 0, 2, 2, 1, 3, 3};
    assert(findLHS(nums, sizeof(nums) / sizeof(int)) == 6);
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
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
    }

    return 0;
}

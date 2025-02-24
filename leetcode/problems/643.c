#include <assert.h>
#include <stdio.h>

double findMaxAverage(int *nums, int numsSize, int k)
{
    if (numsSize < k)
    {
        return 0;
    }

    int max_sum = 0;
    for (int i = 0; i < k; i++)
    {
        max_sum += nums[i];
    }

    int sum = max_sum;
    for (int i = k; i < numsSize; i++)
    {
        sum -= nums[i - k];
        sum += nums[i];

        if (max_sum < sum)
        {
            max_sum = sum;
        }
    }

    return (double)max_sum / k;
}

void test1()
{
    int nums[] = {1, 12, -5, -6, 50, 3};
    int k = 4;

    assert(findMaxAverage(nums, sizeof(nums) / sizeof(int), k) == 12.75000);
}

void test2()
{
    int nums[] = {5};
    int k = 1;

    assert(findMaxAverage(nums, sizeof(nums) / sizeof(int), k) == 5.00000);
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
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
    }

    return 0;
}

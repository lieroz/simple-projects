#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*char containsNearbyDuplicate(int *nums, int numsSize, int k)*/
/*{*/
/*    for (int i = 0; i < numsSize - 1; i++)*/
/*    {*/
/*        for (int j = i + 1; j <= i + k; j++)*/
/*        {*/
/*            if (nums[i] == nums[j])*/
/*            {*/
/*                return 1;*/
/*            }*/
/*        }*/
/*    }*/
/**/
/*    return 0;*/
/*}*/

char containsNearbyDuplicate(int *nums, int numsSize, int k)
{
    int *hashTable = (int *)malloc(sizeof(int) * numsSize);
    memset(hashTable, -1, sizeof(int) * numsSize);

    for (int i = 0; i < numsSize; i++)
    {
        int hash = abs(nums[i]) % numsSize;

        if (hashTable[hash] != -1 && nums[hashTable[hash]] == nums[i])
        {
            if ((i - (hashTable[hash])) <= k)
            {
                free(hashTable);
                return 1;
            }
        }
        hashTable[hash] = i;
    }

    free(hashTable);
    return 0;
}

void test1()
{
    int nums[] = {1, 2, 3, 1};
    int k = 3;

    assert(containsNearbyDuplicate(nums, sizeof(nums) / sizeof(int), k) == 1);
}

void test2()
{
    int nums[] = {1, 0, 1, 1};
    int k = 1;

    assert(containsNearbyDuplicate(nums, sizeof(nums) / sizeof(int), k) == 1);
}

void test3()
{
    int nums[] = {1, 2, 3, 1, 2, 3};
    int k = 2;

    assert(containsNearbyDuplicate(nums, sizeof(nums) / sizeof(int), k) == 0);
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
        test2();
        break;
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
    }

    return 0;
}

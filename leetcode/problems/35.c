#include <assert.h>
#include <stdio.h>

int searchInsert(int* nums, int numsSize, int target)
{
	int begin = 0;
	int mid = 0;
	int end = numsSize - 1;

	while (begin <= end)
	{
		mid = (end + begin) >> 1;

		if (nums[mid] == target)
		{
			return mid;
		}

		if (nums[mid] < target)
		{
			begin = mid + 1;
		}
		else
		{
			end = mid - 1;
		}
	}

	return nums[mid] > target ? mid : mid + 1;
}

void test1()
{
	int nums[] = {1, 3, 5, 6};
	int result = searchInsert(nums, sizeof(nums) / sizeof(*nums), 5);
	printf("result: %d\n", result);
	assert(result == 2);
}

void test2()
{
	int nums[] = {1, 3, 5, 6};
	int result = searchInsert(nums, sizeof(nums) / sizeof(*nums), 2);
	printf("result: %d\n", result);
	assert(result == 1);
}

void test3()
{
	int nums[] = {1, 3, 5, 6};
	int result = searchInsert(nums, sizeof(nums) / sizeof(*nums), 7);
	printf("result: %d\n", result);
	assert(result == 4);
}

void test4()
{
	int nums[] = {1, 3, 5, 6};
	int result = searchInsert(nums, sizeof(nums) / sizeof(*nums), 0);
	printf("result: %d\n", result);
	assert(result == 0);
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
    default:
        printf("Test #%d doesn't exist.\n", choice);
        return -1;
	}
 
	return 0;
}

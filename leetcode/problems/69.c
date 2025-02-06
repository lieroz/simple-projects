#include <assert.h>
#include <stdio.h>

int mySqrt(int x)
{
	long long begin = 1;
	long long end = x;

	while (begin <= end)
	{
		long long mid = (begin + end) >> 1;
		if (mid * mid <= x)
		{
			while (end * end > x)
			{
				end--;
			}
			return end;
		}
		else
		{
			end = mid - 1;
		}
	}

	return 0;
}

void test1()
{
	int result = mySqrt(4);
	assert(result == 2);
}

void test2()
{
	int result = mySqrt(8);
	assert(result == 2);
}

void test3()
{
	int result = mySqrt(1);
	assert(result == 1);
}

void test4()
{
	int result = mySqrt(5);
	assert(result == 2);
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

#include "sliding_window.h"

int find_max_sum_of_all_subarrays_of_size_k(int* array, int size, int k)
{
	if (size < k)
	{
		return -1;
	}

	int max_sum = 0;
	for (int i = 0; i < k; i++)
	{
		max_sum += array[i];
	}

	int sum = max_sum;
	for (int i = k; i < size; i++)
	{
		sum += array[i] - array[i - k];
		if (sum > max_sum)
		{
			max_sum = sum;
		}
	}

	return max_sum;
}

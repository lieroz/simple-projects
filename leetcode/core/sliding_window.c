#include "sliding_window.h"

#include <limits.h>
#include <string.h>

int find_max_sum_of_all_subarrays_of_size_k(int *array, int size, int k)
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

int find_smallest_subarray_with_sum_greater_that_value(int *array, int size, int x)
{
    int begin = 0;
    int end = 0;

    int min_length = INT_MAX;
    int sum = 0;

    while (end < size)
    {
        while (end < size && sum <= x)
        {
            sum += array[end++];
        }

        if (end == size && sum <= x)
        {
            break;
        }

        while (begin < end && sum - array[begin] > x)
        {
            sum -= array[begin++];
        }

        int length = end - begin;
        if (min_length > length)
        {
            min_length = length;
        }

        sum -= array[begin];
        begin++;
    }

    if (min_length == INT_MAX)
    {
        return 0;
    }

    return min_length;
}

void find_subarray_with_given_sum_in_an_array_of_non_negative_integers(int *array, int size, int sum, int indexes[2])
{
    indexes[0] = -1;
    indexes[1] = -1;

    int left = 0;
    int right = 0;

    while (right < size)
    {
        while (sum > 0)
        {
            sum -= array[right++];
        }

        if (sum == 0)
        {
            if (left < right)
            {
                indexes[0] = left;
                indexes[1] = right;
            }
            return;
        }

        while (sum < 0)
        {
            sum += array[left++];
        }
    }
}

void find_smallest_window_that_contains_all_characters_of_string_itself(char *string, int length, int indexes[2])
{
	indexes[0] = 0;
	indexes[1] = 0;

	if (length <= 1)
	{
		return;
	}

	char ascii[256] = {};
	memset(ascii, 0, 256);

	int variance = 0;
	for (int i =  0; i < length; i++)
	{
		char *value = &ascii[string[i]];
		if (*value == 0)
		{
			*value = 1;
			variance++;
		}
	}

	int start = 0;
	int min_length = INT_MAX;
	int count = 0;
	int curr_count[256] = {};
	memset(curr_count, 0, 256 * sizeof(int));

	for (int i = 0; i < length; i++)
	{
		curr_count[string[i]]++;
		if (curr_count[string[i]] == 1)
		{
			count++;
		}

		if (count == variance)
		{
			while (curr_count[string[start]] > 1)
			{
				curr_count[string[start]]--;
				start++;
			}

			int window_length = i - start + 1;
			if (min_length > window_length)
			{
				min_length = window_length;
				indexes[0] = start;
				indexes[1] = start + min_length;
			}
		}
	}
}

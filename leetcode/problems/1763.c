#include <assert.h>
#include <stdio.h>
#include <string.h>

char isNiceSubstring(char *s, int begin, int end)
{
	char ascii[256] = {};
	memset(ascii, 0, 256);

	for (int i = begin; i <= end; i++)
	{
		ascii[s[i]] = 1;
	}
	
	for (int i = begin; i <= end; i++)
	{
		if (ascii[s[i] + 32] == 1 || ascii[s[i] - 32] == 1)
		{
			continue;
		}
		return 0;
	}

	return 1;
}

char *longestNiceSubstring(char *s)
{
	int length = strlen(s);
	int begin = 0;
	int max_len = 0;

	for (int i = 0; i < length; i++)
	{
		for (int j = i; j < length; j++)
		{
			if (isNiceSubstring(s, i, j) == 1)
			{
				int sub_length = j - i + 1;
				if (max_len < sub_length)
				{
					max_len = sub_length;
					begin = i;
				}
			}
		}
	}

	char *sub = s + begin;
	*(sub + max_len) = '\0';
	return sub;
}

void test1()
{
	char *s = "YazaAay";
	char *sub = longestNiceSubstring(s);
	assert(strlen(sub) == 3 && strcmp(sub, "aAa") == 0);
}

void test2()
{
	char *s = "Bb";
	char *sub = longestNiceSubstring(s);
	assert(strlen(sub) == 2 && strcmp(sub, "Bb") == 0);
}

void test3()
{
	char *s = "c";
	char *sub = longestNiceSubstring(s);
	assert(strlen(sub) == 0);
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

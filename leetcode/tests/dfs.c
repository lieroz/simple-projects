#include "../core/dfs.h"

#include <stdio.h>

void test1()
{
	int graph[] = {
		0, 1, 0, 0, 0,
		1, 0, 1, 0, 0,
		1, 1, 0, 1, 1,
		0, 0, 1, 0, 0,
		0, 0, 1, 0, 0,
	};
	enum { Graph_Size = 5 };
	char visited[Graph_Size] = {0};
	dfs(graph, Graph_Size, visited);
}

void test2()
{
	int graph[] = {
		0, 1, 0, 0, 0,
		1, 0, 1, 0, 0,
		1, 1, 0, 0, 0,
		0, 0, 0, 0, 1,
		0, 0, 0, 1, 0,
	};
	enum { Graph_Size = 5 };
	char visited[Graph_Size] = {0};
	dfs(graph, Graph_Size, visited);
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

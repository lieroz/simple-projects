#include "dfs.h"

#include <stdio.h>

void dfs_recursive(int *graph, int size, int node, char *visited)
{
    if (visited[node] == 1)
    {
        return;
    }

    // TODO: add to vector after implementing one
    printf("%d\n", node);
    visited[node] = 1;

    for (int i = 0; i < size; i++)
    {
        if (graph[node * size + i] == 1)
        {
            dfs_recursive(graph, size, i, visited);
        }
    }
}

void dfs(int *graph, int size, char *visited)
{
    for (int i = 0; i < size; i++)
    {
        dfs_recursive(graph, size, i, visited);
    }
}

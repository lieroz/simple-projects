#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TreeNode
{
    int val;
    struct TreeNode *left;
    struct TreeNode *right;
};

struct TreeNode *new_node(int val)
{
    struct TreeNode *node = malloc(sizeof(struct TreeNode));
    node->val = val;
    node->left = node->right = NULL;
    return node;
}

void inorder_traversal(struct TreeNode *root, int *count)
{
    if (root == NULL)
    {
        return;
    }

    inorder_traversal(root->left, count);
    (*count)++;

    inorder_traversal(root->right, count);
}

int countNodes(struct TreeNode *root)
{
    int count = 0;
    inorder_traversal(root, &count);
    return count;
}

void test1()
{
    struct TreeNode *n1 = new_node(1);
    struct TreeNode *n2 = new_node(2);
    struct TreeNode *n3 = new_node(3);
    struct TreeNode *n4 = new_node(4);
    struct TreeNode *n5 = new_node(5);
    struct TreeNode *n6 = new_node(6);

    n1->left = n2;
    n1->right = n3;

    n2->left = n4;
    n2->right = n5;

    n3->left = n6;

    assert(countNodes(n1) == 6);
}

void test2()
{
    assert(countNodes(NULL) == 0);
}

void test3()
{
    struct TreeNode *n1 = new_node(1);

    assert(countNodes(n1) == 1);
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

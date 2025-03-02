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

void inorder_traversal(struct TreeNode *root, int *returnSize, int *result)
{
    if (root == NULL)
    {
        return;
    }

    inorder_traversal(root->left, returnSize, result);
    result[(*returnSize)++] = root->val;

    inorder_traversal(root->right, returnSize, result);
}

int *inorderTraversal(struct TreeNode *root, int *returnSize)
{
    int temp[100];

    inorder_traversal(root, returnSize, temp);

    int size = *returnSize * sizeof(int);
    int *result = malloc(size);
    memcpy_s(result, size, temp, size);
    return result;
}

void test1()
{
    struct TreeNode *n1 = new_node(1);
    struct TreeNode *n2 = new_node(2);
    struct TreeNode *n3 = new_node(3);

    n1->right = n2;
    n2->left = n3;

    int ref[] = {1, 3, 2};

    int count = 0;
    int *result = inorderTraversal(n1, &count);

    assert(count == sizeof(ref) / sizeof(int));

    for (int i = 0; i < count; i++)
    {
        assert(ref[i] == result[i]);
    }
}

void test2()
{
    struct TreeNode *n1 = new_node(1);
    struct TreeNode *n2 = new_node(2);
    struct TreeNode *n3 = new_node(3);

    n1->left = n2;
    n1->right = n3;

    struct TreeNode *n4 = new_node(4);
    struct TreeNode *n5 = new_node(5);

    n2->left = n4;
    n2->right = n5;

    struct TreeNode *n6 = new_node(6);
    struct TreeNode *n7 = new_node(7);

    n5->left = n6;
    n5->right = n7;

    struct TreeNode *n8 = new_node(8);

    n3->right = n8;

    struct TreeNode *n9 = new_node(9);

    n8->left = n9;

    int ref[] = {4, 2, 6, 5, 7, 1, 3, 9, 8};

    int count = 0;
    int *result = inorderTraversal(n1, &count);

    assert(count == sizeof(ref) / sizeof(int));

    for (int i = 0; i < count; i++)
    {
        assert(ref[i] == result[i]);
    }
}

void test3()
{
    int ref[] = {};

    int count = 0;
    int *result = inorderTraversal(NULL, &count);

    assert(count == 0);
}

void test4()
{
    struct TreeNode *n1 = new_node(1);

    int ref[] = {1};

    int count = 0;
    int *result = inorderTraversal(n1, &count);

    assert(count == sizeof(ref) / sizeof(int));

    for (int i = 0; i < count; i++)
    {
        assert(ref[i] == result[i]);
    }
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

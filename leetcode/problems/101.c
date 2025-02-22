#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

char dfs(struct TreeNode *p, struct TreeNode *q)
{
    if (p == NULL && q == NULL)
    {
        return 1;
    }

    if (p == NULL || q == NULL)
    {
        return 0;
    }

    if (p->val == q->val)
    {
        char equal = dfs(p->right, q->left);
        if (equal == 0)
        {
            return 0;
        }

        equal = dfs(p->left, q->right);
        if (equal == 0)
        {
            return 0;
        }

        return 1;
    }

    return 0;
}

char isSymmetric(struct TreeNode *root)
{
    char equal = dfs(root->left, root->right);
    return equal;
}

void test1()
{
    struct TreeNode *n1 = new_node(1);
    struct TreeNode *n2left = new_node(2);
    struct TreeNode *n2right = new_node(2);

    n1->left = n2left;
    n1->right = n2right;

    struct TreeNode *n3left = new_node(3);
    struct TreeNode *n4left = new_node(4);

    struct TreeNode *n3right = new_node(3);
    struct TreeNode *n4right = new_node(4);

    n2left->left = n3left;
    n2left->right = n4left;

    n2right->left = n4right;
    n2right->right = n3right;

    assert(isSymmetric(n1) == 1);
}

void test2()
{
    struct TreeNode *n1 = new_node(1);
    struct TreeNode *n2left = new_node(2);
    struct TreeNode *n2right = new_node(2);

    n1->left = n2left;
    n1->right = n2right;

    struct TreeNode *n3left = new_node(3);
    struct TreeNode *n3right = new_node(3);

    n2left->right = n3left;
    n2right->right = n3right;

    assert(isSymmetric(n1) == 0);
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

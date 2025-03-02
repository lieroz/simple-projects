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

struct TreeNode *insert(struct TreeNode *node, int val)
{
    if (node == NULL)
    {
        return new_node(val);
    }

    if (node->val <= val)
    {
        node->left = insert(node->left, val);
    }
    else
    {
        node->right = insert(node->right, val);
    }

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
        char equal = dfs(p->left, q->left);
        if (equal == 0)
        {
            return 0;
        }

        equal = dfs(p->right, q->right);
        if (equal == 0)
        {
            return 0;
        }

        return 1;
    }

    return 0;
}

char isSameTree(struct TreeNode *p, struct TreeNode *q)
{
    return dfs(p, q);
}

void test1()
{
    struct TreeNode *root1 = insert(NULL, 1);
    struct TreeNode *left1 = insert(root1, 2);
    struct TreeNode *right1 = insert(root1, 3);

    struct TreeNode *root2 = insert(NULL, 1);
    struct TreeNode *left2 = insert(root2, 2);
    struct TreeNode *right2 = insert(root2, 3);

    assert(isSameTree(root1, root2) == 1);
}

void test2()
{
    struct TreeNode *root1 = insert(NULL, 1);
    struct TreeNode *left1 = insert(root1, 2);

    struct TreeNode *root2 = insert(NULL, 1);
    struct TreeNode *left2 = insert(root2, 0);
    struct TreeNode *right2 = insert(root2, 2);

    assert(isSameTree(root1, root2) == 0);
}

void test3()
{
    struct TreeNode *root1 = insert(NULL, 1);
    struct TreeNode *left1 = insert(root1, 2);
    struct TreeNode *right1 = insert(root1, 1);

    struct TreeNode *root2 = insert(NULL, 1);
    struct TreeNode *left2 = insert(root2, 1);
    struct TreeNode *right2 = insert(root2, 2);

    assert(isSameTree(root1, root2) == 0);
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

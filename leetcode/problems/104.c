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

int maxDepth(struct TreeNode* root)
{
	if (root == NULL)
	{
		return 0;
	}

	static struct TreeNode *queue[10000];
	int begin = 0;
	int end = 0;

	queue[end++] = root;
    root->val = 1;

	while (end - begin > 0)
	{
		struct TreeNode *node = queue[begin++];
		struct TreeNode *left = node->left;
		struct TreeNode *right = node->right;

		if (left != NULL)
		{
			queue[end++] = left;
            left->val = node->val + 1;
            root = left;
		}

		if (right != NULL)
		{
			queue[end++] = right;
            right->val = node->val + 1;
            root = right;
		}
	}

	return root->val;
}

void test1()
{
	struct TreeNode *n1 = new_node(3);
	struct TreeNode *n2 = new_node(9);
	struct TreeNode *n3 = new_node(20);
	struct TreeNode *n4 = new_node(15);
	struct TreeNode *n5 = new_node(7);

	n1->left = n2;
	n1->right = n3;
	n3->left = n4;
	n3->right = n5;

	assert(maxDepth(n1) == 3);
}

void test2()
{
	struct TreeNode *n1 = new_node(1);
	struct TreeNode *n2 = new_node(2);

	n1->right = n2;

	assert(maxDepth(n1) == 2);
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

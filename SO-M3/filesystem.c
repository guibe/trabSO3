#include "filesystem.h"
#include <limits.h>

static Directory* root_dir = NULL;

Directory* get_root_directory(void)
{
    if (root_dir == NULL)
    {
        root_dir = (Directory*)malloc(sizeof(Directory));
        root_dir->tree = btree_create(DEFAULT_T);
    }
    return root_dir;
}

BTreeNode* create_btree_node(int t, bool leaf)
{
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    node->n = 0;
    node->leaf = leaf;
    node->keys = (char**)malloc(sizeof(char*) * (2 * t - 1));
    node->values = (TreeNode**)malloc(sizeof(TreeNode*) * (2 * t - 1));
    node->children = (BTreeNode**)calloc(2 * t, sizeof(BTreeNode*));
    return node;
}

BTree* btree_create(int t)
{
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    tree->t = t;
    tree->root = create_btree_node(t, true);
    return tree;
}

TreeNode* btree_search(BTree* tree, const char* name)
{
    if (tree == NULL || tree->root == NULL)
        return NULL;

    BTreeNode* current = tree->root;
    while (current != NULL)
    {
        int i = 0;
        while (i < current->n && strcmp(name, current->keys[i]) > 0)
            i++;

        if (i < current->n && strcmp(name, current->keys[i]) == 0)
            return current->values[i];

        if (current->leaf)
            break;
        else
            current = current->children[i];
    }
    return NULL;
}

void split_child(BTreeNode* parent, int index, int t)
{
    BTreeNode* child = parent->children[index];
    BTreeNode* new_child = create_btree_node(t, child->leaf);
    new_child->n = t - 1;

    for (int j = 0; j < t - 1; j++)
    {
        new_child->keys[j] = child->keys[j + t];
        new_child->values[j] = child->values[j + t];
    }

    if (!child->leaf)
    {
        for (int j = 0; j < t; j++)
            new_child->children[j] = child->children[j + t];
    }

    child->n = t - 1;

    for (int j = parent->n; j > index; j--)
        parent->children[j + 1] = parent->children[j];
    parent->children[index + 1] = new_child;

    for (int j = parent->n - 1; j >= index; j--)
    {
        parent->keys[j + 1] = parent->keys[j];
        parent->values[j + 1] = parent->values[j];
    }

    parent->keys[index] = child->keys[t - 1];
    parent->values[index] = child->values[t - 1];
    parent->n += 1;
}

void insert_non_full(BTreeNode* node, TreeNode* value, int t)
{
    int i = node->n - 1;

    if (node->leaf)
    {
        while (i >= 0 && strcmp(value->name, node->keys[i]) < 0)
        {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        node->keys[i + 1] = strdup(value->name);
        node->values[i + 1] = value;
        node->n += 1;
    }
    else
    {
        while (i >= 0 && strcmp(value->name, node->keys[i]) < 0)
            i--;
        i++;

        if (node->children[i]->n == 2 * t - 1)
        {
            split_child(node, i, t);
            if (strcmp(value->name, node->keys[i]) > 0)
                i++;
        }
        insert_non_full(node->children[i], value, t);
    }
}

void btree_insert(BTree* tree, TreeNode* node)
{
    if (tree == NULL)
        return;

    BTreeNode* root = tree->root;

    if (root->n == 2 * tree->t - 1)
    {
        BTreeNode* new_root = create_btree_node(tree->t, false);
        new_root->children[0] = root;
        tree->root = new_root;
        split_child(new_root, 0, tree->t);
        insert_non_full(new_root, node, tree->t);
    }
    else
    {
        insert_non_full(root, node, tree->t);
    }
}

TreeNode* get_predecessor(BTreeNode* node, int idx)
{
    BTreeNode* cur = node->children[idx];
    while (!cur->leaf)
        cur = cur->children[cur->n];
    return cur->values[cur->n - 1];
}

TreeNode* get_successor(BTreeNode* node, int idx)
{
    BTreeNode* cur = node->children[idx + 1];
    while (!cur->leaf)
        cur = cur->children[0];
    return cur->values[0];
}

void merge_nodes(BTreeNode* parent, int idx, int t)
{
    BTreeNode* left = parent->children[idx];
    BTreeNode* right = parent->children[idx + 1];

    left->keys[left->n] = strdup(parent->keys[idx]);
    left->values[left->n] = parent->values[idx];
    left->n++;

    for (int i = 0; i < right->n; i++)
    {
        left->keys[left->n + i] = right->keys[i];
        left->values[left->n + i] = right->values[i];
    }

    if (!left->leaf)
    {
        for (int i = 0; i <= right->n; i++)
            left->children[left->n + i] = right->children[i];
    }
    left->n += right->n;

    for (int i = idx; i < parent->n - 1; i++)
    {
        parent->keys[i] = parent->keys[i + 1];
        parent->values[i] = parent->values[i + 1];
    }
    for (int i = idx + 1; i < parent->n; i++)
        parent->children[i] = parent->children[i + 1];
    parent->n--;

    free(right->keys);
    free(right->values);
    free(right->children);
    free(right);
}

void remove_from_leaf(BTreeNode* node, int idx)
{
    free(node->keys[idx]);
    for (int i = idx + 1; i < node->n; i++)
    {
        node->keys[i - 1] = node->keys[i];
        node->values[i - 1] = node->values[i];
    }
    node->n--;
}

void remove_from_non_leaf(BTree* tree, BTreeNode* node, int idx, int t)
{
    TreeNode* k = node->values[idx];

    if (node->children[idx]->n >= t)
    {
        TreeNode* pred = get_predecessor(node, idx);
        node->values[idx] = pred;
        btree_delete(tree, pred->name);
    }
    else if (node->children[idx + 1]->n >= t)
    {
        TreeNode* succ = get_successor(node, idx);
        node->values[idx] = succ;
        btree_delete(tree, succ->name);
    }
    else
    {
        merge_nodes(node, idx, t);
        btree_delete(tree, k->name);
    }
}

void delete_from_node(BTree* tree, BTreeNode* node, const char* name, int t)
{
    int idx = 0;
    while (idx < node->n && strcmp(name, node->keys[idx]) > 0)
        idx++;

    if (idx < node->n && strcmp(name, node->keys[idx]) == 0)
    {
        if (node->leaf)
            remove_from_leaf(node, idx);
        else
            remove_from_non_leaf(tree, node, idx, t);
    }
    else
    {
        if (node->leaf)
            return;

        bool is_last = (idx == node->n);

        if (node->children[idx]->n < t)
        {
            if (idx > 0 && node->children[idx - 1]->n >= t)
            {
                BTreeNode* child = node->children[idx];
                BTreeNode* sibling = node->children[idx - 1];

                for (int i = child->n - 1; i >= 0; i--)
                {
                    child->keys[i + 1] = child->keys[i];
                    child->values[i + 1] = child->values[i];
                }

                if (!child->leaf)
                {
                    for (int i = child->n; i >= 0; i--)
                        child->children[i + 1] = child->children[i];
                }

                child->keys[0] = node->keys[idx - 1];
                child->values[0] = node->values[idx - 1];

                if (!child->leaf)
                    child->children[0] = sibling->children[sibling->n];

                node->keys[idx - 1] = sibling->keys[sibling->n - 1];
                node->values[idx - 1] = sibling->values[sibling->n - 1];

                child->n++;
                sibling->n--;
            }
            else if (idx < node->n && node->children[idx + 1]->n >= t)
            {
                BTreeNode* child = node->children[idx];
                BTreeNode* sibling = node->children[idx + 1];

                child->keys[child->n] = node->keys[idx];
                child->values[child->n] = node->values[idx];

                if (!child->leaf)
                    child->children[child->n + 1] = sibling->children[0];

                node->keys[idx] = sibling->keys[0];
                node->values[idx] = sibling->values[0];

                for (int i = 1; i < sibling->n; i++)
                {
                    sibling->keys[i - 1] = sibling->keys[i];
                    sibling->values[i - 1] = sibling->values[i];
                }

                if (!sibling->leaf)
                {
                    for (int i = 1; i <= sibling->n; i++)
                        sibling->children[i - 1] = sibling->children[i];
                }

                child->n++;
                sibling->n--;
            }
            else
            {
                if (idx < node->n)
                    merge_nodes(node, idx, t);
                else
                    merge_nodes(node, idx - 1, t);
            }
        }

        if (is_last && idx > node->n)
            delete_from_node(tree, node->children[idx - 1], name, t);
        else
            delete_from_node(tree, node->children[idx], name, t);
    }
}

void btree_delete(BTree* tree, const char* name)
{
    if (tree == NULL || tree->root == NULL)
        return;

    delete_from_node(tree, tree->root, name, tree->t);

    if (tree->root->n == 0)
    {
        BTreeNode* old_root = tree->root;
        if (old_root->leaf)
            tree->root = NULL;
        else
            tree->root = old_root->children[0];
        free(old_root->keys);
        free(old_root->values);
        free(old_root->children);
        free(old_root);
    }
}

void btree_traverse_util(BTreeNode* node)
{
    if (node != NULL)
    {
        int i;
        for (i = 0; i < node->n; i++)
        {
            if (!node->leaf)
                btree_traverse_util(node->children[i]);
            printf("%s\n", node->keys[i]);
        }
        if (!node->leaf)
            btree_traverse_util(node->children[i]);
    }
}

void btree_traverse(BTree* tree)
{
    if (tree && tree->root)
        btree_traverse_util(tree->root);
}

TreeNode* create_txt_file(const char* name, const char* content)
{
    if (strlen(content) > MAX_CONTENT_SIZE)
    {
        fprintf(stderr, "Conteúdo excede 1MB\n");
        return NULL;
    }

    File* file = (File*)malloc(sizeof(File));
    file->name = strdup(name);
    file->content = strdup(content);
    file->size = strlen(content);

    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    node->name = strdup(name);
    node->type = FILE_TYPE;
    node->data.file = file;

    return node;
}

void delete_txt_file(BTree* tree, const char* name)
{
    TreeNode* node = btree_search(tree, name);
    if (node == NULL || node->type != FILE_TYPE)
    {
        fprintf(stderr, "Arquivo não encontrado: %s\n", name);
        return;
    }

    btree_delete(tree, name);

    free(node->data.file->name);
    free(node->data.file->content);
    free(node->data.file);
    free(node->name);
    free(node);
}

TreeNode* create_directory(const char* name)
{
    Directory* dir = (Directory*)malloc(sizeof(Directory));
    dir->tree = btree_create(DEFAULT_T);

    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    node->name = strdup(name);
    node->type = DIRECTORY_TYPE;
    node->data.directory = dir;

    return node;
}

void delete_directory(BTree* tree, const char* name)
{
    TreeNode* node = btree_search(tree, name);
    if (node == NULL || node->type != DIRECTORY_TYPE)
    {
        fprintf(stderr, "Diretório não encontrado: %s\n", name);
        return;
    }

    Directory* dir = node->data.directory;
    if (dir->tree->root->n > 0)
    {
        fprintf(stderr, "Diretório não está vazio: %s\n", name);
        return;
    }

    btree_delete(tree, name);
    free(dir->tree->root->keys);
    free(dir->tree->root->values);
    free(dir->tree->root->children);
    free(dir->tree->root);
    free(dir->tree);
    free(dir);
    free(node->name);
    free(node);
}

void change_directory(Directory** current, const char* path)
{
    if (strcmp(path, "/") == 0)
    {
        *current = get_root_directory();
        return;
    }

    char* path_copy = strdup(path);
    char* token = strtok(path_copy, "/");
    Directory* temp = *current;

    if (path[0] == '/')
        temp = get_root_directory();

    while (token != NULL)
    {
        TreeNode* node = btree_search(temp->tree, token);
        if (node == NULL || node->type != DIRECTORY_TYPE)
        {
            fprintf(stderr, "Diretório não encontrado: %s\n", token);
            free(path_copy);
            return;
        }
        temp = node->data.directory;
        token = strtok(NULL, "/");
    }

    *current = temp;
    free(path_copy);
}

void list_directory_contents(Directory* dir)
{
    if (dir && dir->tree)
        btree_traverse(dir->tree);
}

void free_tree_node(BTreeNode* node)
{
    if (node)
    {
        for (int i = 0; i < node->n; i++)
            free(node->keys[i]);

        free(node->keys);
        free(node->values);

        if (!node->leaf)
        {
            for (int i = 0; i <= node->n; i++)
                if (node->children[i])
                    free_tree_node(node->children[i]);
        }
        free(node->children);
        free(node);
    }
}

void free_directory_recursive(Directory* dir)
{
    if (dir)
    {
        if (dir->tree && dir->tree->root)
        {
            BTreeNode* root = dir->tree->root;
            for (int i = 0; i < root->n; i++)
            {
                if (root->values[i]->type == DIRECTORY_TYPE)
                    free_directory_recursive(root->values[i]->data.directory);
                else
                {
                    free(root->values[i]->data.file->name);
                    free(root->values[i]->data.file->content);
                    free(root->values[i]->data.file);
                    free(root->values[i]->name);
                    free(root->values[i]);
                }
            }
            free_tree_node(root);
        }
        free(dir->tree);
        free(dir);
    }
}

void cleanup_fs()
{
    if (root_dir)
    {
        free_directory_recursive(root_dir);
        root_dir = NULL;
    }
}

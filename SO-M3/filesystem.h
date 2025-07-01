#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define DEFAULT_T 3
#define MAX_NAME_LEN 256
#define MAX_PATH_LEN 1024
#define MAX_CONTENT_SIZE 1048576

typedef enum { FILE_TYPE, DIRECTORY_TYPE } NodeType;

typedef struct File {
    char* name;
    char* content;
    size_t size;
} File;

typedef struct Directory Directory;

typedef struct TreeNode {
    char* name;
    NodeType type;
    union {
        File* file;
        Directory* directory;
    } data;
} TreeNode;

typedef struct BTreeNode {
    int n;
    bool leaf;
    char **keys;
    TreeNode **values;
    struct BTreeNode **children;
} BTreeNode;

typedef struct BTree {
    BTreeNode *root;
    int t;
} BTree;

struct Directory {
    BTree* tree;
};

// BTree Functions
BTree* btree_create(int t);
void btree_insert(BTree* tree, TreeNode* node);
void btree_delete(BTree* tree, const char* name);
TreeNode* btree_search(BTree* tree, const char* name);
void btree_traverse(BTree* tree);

// File/Directory creation
TreeNode* create_txt_file(const char* name, const char* content);
TreeNode* create_directory(const char* name);
void delete_txt_file(BTree* tree, const char* name);
void delete_directory(BTree* tree, const char* name);

// Navigation
Directory* get_root_directory(void);
void change_directory(Directory** current, const char* path);
void list_directory_contents(Directory* dir);

// Cleanup
void cleanup_fs(void);

#endif

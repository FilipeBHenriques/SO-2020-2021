#ifndef FS_H
#define FS_H
#include "state.h"
#include <unistd.h>
#include <pthread.h>
void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name);
void print_tecnicofs_tree(int argc, char* argv[]);

#endif /* FS_H */

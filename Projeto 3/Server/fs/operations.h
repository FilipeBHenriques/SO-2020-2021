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
int print_tecnicofs_tree(char *outputfile);
int move(char *old_path, char *new_path);
#endif /* FS_H */

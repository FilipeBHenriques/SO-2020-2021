#ifndef FS_H
#define FS_H
#include "state.h"
#include <unistd.h>
#include <pthread.h>
#define WRITE 7
#define READ 9
#define NOLOCKS 11
void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType,int locks[INODE_TABLE_SIZE],int *n_locks);
int delete(char *name,int locks[INODE_TABLE_SIZE], int *n_locks);
int lookup(char *name,int locks[INODE_TABLE_SIZE], int *n_locks,int flag,char *comum);
int move(char *old_path, char *new_path,int *n_locks, int locks[INODE_TABLE_SIZE]);
void print_tecnicofs_tree(int argc, char* argv[]);

#endif /* FS_H */

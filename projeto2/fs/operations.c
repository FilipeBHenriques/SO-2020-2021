#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
pthread_mutex_t Lockcheck;


void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType,int locks[INODE_TABLE_SIZE],int *n_locks){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	printf(" Pai :%s  Filho : %s\n",parent_name,child_name);
	parent_inumber = lookup(parent_name,locks,n_locks,WRITE,NULL);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		return FAIL;
	}
	

	inode_get(parent_inumber, &pType, &pdata); 

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	
	
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name,int locks[INODE_TABLE_SIZE],int *n_locks){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name,locks,n_locks,WRITE,NULL);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		return FAIL;
	}
	
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		return FAIL;
	}
	pthread_rwlock_wrlock(&inode_table[child_inumber].rwlock);
	inode_get(child_inumber, &cType, &cdata);
	(*n_locks)++;
	locks[*n_locks] = child_inumber;

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		return FAIL;
	}

	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, int locks[INODE_TABLE_SIZE],int *n_locks,int flag,char *comum) {
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	
	char *saveptr;
	char *saveptr2;
	int i = 0;
	
	char comum2[MAX_FILE_NAME];
	char *comum3;

	strcpy(full_path, name);
	if(comum != NULL){ strcpy(comum2,comum);}

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;
	
	char *path = strtok_r(full_path, delim, &saveptr);
	/* get root inode data */
	if(comum != NULL){comum3 = strtok_r(comum2, delim, &saveptr2);}
	if(path == NULL && flag == WRITE ) {pthread_rwlock_wrlock(&inode_table[current_inumber].rwlock);} /* Se for create de apenas uma "/" */
	else if (path !=NULL || flag == READ) {pthread_rwlock_rdlock(&inode_table[current_inumber].rwlock);}
	locks[*n_locks] = current_inumber ; 	
	inode_get(current_inumber, &nType, &data);

	
	
	
		
    
    
	/* search for all sub nodes */     
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		if (comum != NULL){
			if(strcmp(path,comum3) == 0){  /* Para evitar locks duplos do move */
				i = 1;
			}
			if(strcmp(path,comum3) != 0){
				i = 0;
			}
			if(comum3 != NULL){comum3 = strtok_r(NULL, delim, &saveptr2);}
		}
		
		path = strtok_r(NULL, delim, &saveptr);
		
		if( path == NULL && flag == WRITE && i != 1){
			pthread_rwlock_wrlock(&inode_table[current_inumber].rwlock);
			(*n_locks)++;
			locks[*n_locks] = current_inumber;
				
		}
		else if ((flag == READ || path != NULL) && i != 1){
			pthread_rwlock_rdlock(&inode_table[current_inumber].rwlock);
			(*n_locks)++;
			locks[*n_locks] = current_inumber;
				
		}
			
		inode_get(current_inumber, &nType, &data);
		
		
		
		
	}
	return current_inumber;
}



/*
 * Move from old_path to new_path.
 * Input:
 *  - old_path: path to move from
 *  - new_path: path to move to
 *
 *	
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int move(char *old_path, char *new_path,int *n_locks, int locks[INODE_TABLE_SIZE]){
	
	char *parent_name_old; 
	char *child_name_old;
	char *parent_name_new;
	char *child_name_new;
	char comum[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;
	char *saveptr2;
	char save_old_path[MAX_FILE_NAME];
	char save_new_path[MAX_FILE_NAME]; 
	int child_inumber_old;
	int child_inumber_new;
	type pType;
	union Data pdata;
	strcpy(save_old_path,old_path);
	strcpy(save_new_path,new_path);

	split_parent_child_from_path(old_path, &parent_name_old, &child_name_old);
	split_parent_child_from_path(new_path, &parent_name_new, &child_name_new);
	
	char *string1 = strtok_r(save_old_path, delim, &saveptr);
	char *string2 = strtok_r(save_new_path, delim, &saveptr2);
	while(string1 !=NULL && string2 != NULL){ /* Check the common paths between both paths */
		
		if (strcmp(string1,string2) == 0){
			
			strcat(comum,"/");
			strcat(comum,string1);
			
		}
		string1 = strtok_r(NULL, delim, &saveptr);
		string2 = strtok_r(NULL, delim, &saveptr2);
	}
	

	int parent_old_inumber = lookup(parent_name_old,locks,n_locks,READ,NULL);

	int parent_new_inumber = lookup(parent_name_new,locks,n_locks,READ,comum);
	/* Verifications */
	if (parent_old_inumber== FAIL) {
		printf("Does not exist: %s\n",save_old_path);
		return FAIL;
	}
	if (parent_new_inumber != FAIL) {
		printf("Already exist: %s\n",save_new_path);
		return FAIL;
	}
	inode_get(parent_old_inumber,&pType,&pdata);
	child_inumber_old = lookup_sub_node(child_name_old, pdata.dirEntries);
	if (child_inumber_old == FAIL) {
		printf("Does not exist: %s\n",save_old_path);
		return FAIL;
	}
	inode_get(parent_new_inumber,&pType,&pdata);
	child_inumber_new = lookup_sub_node(child_name_new, pdata.dirEntries);
	if (child_inumber_new != FAIL) {
		printf("Already exist: %s\n",save_old_path);
		return FAIL;
	}
	if(dir_reset_entry(parent_old_inumber,child_inumber_old) == FAIL){printf("Could not reset entry\n");}
	if(dir_add_entry(parent_old_inumber,child_inumber_old,child_name_new) ==FAIL){printf("Could not add entry\n");}


	return SUCCESS;
}




/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(int argc, char* argv[]){
	FILE *output;
	output = fopen(argv[2],"w");
	if (output == NULL){
        fprintf(stderr,"Error: Could not open output file\n");
        exit(EXIT_FAILURE);
    }
	inode_print_tree(output, FS_ROOT, "");
	fclose(output);
}

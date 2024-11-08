#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>


#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100



/*NOTA: O projeto corre bem manual mas nao 100% com shell script */

int numberThreads = 0;
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int fill_here = 0;
int use_here = 0;
int global_flag = 15;


int use_ptr = 0;
pthread_mutex_t global_lock;
pthread_cond_t open_space, filled_space;




/*Inserts a Command in inputCommands */
int insertCommand(char* data) {
    
    pthread_mutex_lock(&global_lock);
   
    while(numberCommands == MAX_COMMANDS){
        
        pthread_cond_wait(&open_space,&global_lock);
    }
    if(numberCommands != MAX_COMMANDS) {
       
        strcpy(inputCommands[fill_here], data);
        fill_here++;
        if(fill_here == MAX_COMMANDS){fill_here = 0;}
        numberCommands++;
        
        pthread_cond_signal(&filled_space); 
        
        pthread_mutex_unlock(&global_lock);
        return 1;
        
    }
    pthread_mutex_unlock(&global_lock);
    return 0;
}


char* removeCommand() {
    
    if(numberCommands == 0 && global_flag == -1){
        pthread_cond_signal(&filled_space); /*Para garantir que nenhum fica em espera */
        
        pthread_exit(NULL);
    }
   
    pthread_mutex_lock(&global_lock);
    while (numberCommands == 0 && global_flag == 15)
        pthread_cond_wait(&filled_space,&global_lock);
    
    if(numberCommands > 0){
        
        
        if(use_here == MAX_COMMANDS){use_here = 0;}
        
        numberCommands--;
        
        pthread_cond_signal(&open_space); 
        pthread_mutex_unlock(&global_lock);
        return inputCommands[use_here++];  
    }
    pthread_mutex_unlock(&global_lock);
    
    return NULL;
    
}

/* Error Function */
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* Checks for errors in input */
void verifyInput(int argc, char* argv[]){
    
    if(argc != 4){
        fprintf(stderr, "Error: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    numberThreads = atoi(argv[3]);
    if(numberThreads <= 0){ /*Verificacao do quarto argumento */
        fprintf(stderr,"Error: Fourth argument is not valid.\n");
        exit(EXIT_FAILURE);
    }
    return;

}

/* Opens the file and processes its input */
void processInput(int argc, char* argv[]){
    char line[MAX_INPUT_SIZE];
    FILE *input;
    input = fopen(argv[1],"r");
    if (input == NULL){
        fprintf(stderr,"Error: Could not open file\n");
        exit(EXIT_FAILURE);
    }
    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), input)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'm':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
                
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    global_flag = -1; /* Quando chegar ao final do ficheiro avisa as threads */
    fclose(input);
}

/* Applies all the commands */
void *applyCommands(){
    
    while (numberCommands >= 0){
        
        const char* command = removeCommand();
        
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        char new_path[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int locks[INODE_TABLE_SIZE];
        int i;
        int n_locks= 0;
        
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        
                        printf("Create file: %s\n", name);
                        create(name, T_FILE,locks,&n_locks);
                        
                        for(i = 0; i <= n_locks;i++){
                            if (pthread_rwlock_unlock(&inode_table[locks[i]].rwlock) != 0){
                                break;
                            }
                        }
                        
                        break;
                    case 'd':
                        
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY,locks,&n_locks);
                        for(i = 0; i <= n_locks;i++){
                            if (pthread_rwlock_unlock(&inode_table[locks[i]].rwlock) != 0){
                                break;
                            }
                        }
                        
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                
                searchResult = lookup(name,locks,&n_locks,READ,NULL);
                for(i = 0; i <= n_locks;i++){
                    if (pthread_rwlock_unlock(&inode_table[locks[i]].rwlock) != 0){
                        break;
                    }
                }
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
               
                break;
            case 'd':
                
                printf("Delete: %s\n", name);
                delete(name,locks,&n_locks);
                for(i = 0; i <= n_locks;i++){
                    if (pthread_rwlock_unlock(&inode_table[locks[i]].rwlock) != 0){
                        break;
                    }
                }
                
                break;
            case 'm':
                sscanf(command, "%c %s %s", &token, name,new_path);
                printf("Move: %s to : %s\n", name,new_path);
                move(name,new_path,&n_locks, locks);
                
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}


/* Initializes Threads and creates them;
   Starts the clock */
 void initThread_processinput(int argc, char* argv[]){
    int i;
    pthread_t tid[numberThreads];
    
    gettimeofday(&t1, NULL);
    for (i = 0; i < numberThreads; i++){
        if(pthread_create (&tid[i], 0, applyCommands,NULL) != 0) 
            fprintf(stderr,"Could not create thread.\n");
    }
    
    processInput(argc,argv);
    
    for (i=0; i<numberThreads; i++){
        if(pthread_join (tid[i], NULL) != 0)
            fprintf(stderr,"Could not join thread.\n");
    }
    return;
} 






int main(int argc, char* argv[]) {
   
    verifyInput(argc,argv);
   
    /* init filesystem */
    init_fs();
    
    pthread_mutex_init(&global_lock,NULL);
    gettimeofday(&t1, NULL);
  
    initThread_processinput(argc,argv);
   

    /*print tree */
    print_tecnicofs_tree(argc,argv);

    /* release allocated memory */
    pthread_mutex_destroy(&global_lock);
    destroy_fs();
    
    gettimeofday(&t2, NULL);
    
    getTime(t1,t2);
    
    exit(EXIT_SUCCESS);
} 
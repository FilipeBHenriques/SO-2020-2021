#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>


#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100
#define read 1
#define write 2

int numberThreads = 0;
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;
pthread_mutex_t inputLock;
pthread_mutex_t lock;
pthread_rwlock_t lockrw;

/*Locks critical seccions depending on the syncstrategy */
void checkLOCK(char *sync,int rw){
    if(strcmp(sync,"mutex") == 0){
        if (pthread_mutex_lock(&lock) != 0){
            fprintf(stderr,"Error: Could not lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(sync,"rwlock") == 0){
        
        if (rw == read){
        
            if(pthread_rwlock_rdlock(&lockrw) != 0){
                fprintf(stderr,"Error: Could not lock.\n");
                exit(EXIT_FAILURE);
            }
        }
        else if (rw == write){
            if(pthread_rwlock_wrlock(&lockrw) != 0){
                fprintf(stderr,"Error: Could not lock.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    else if (strcmp(sync,"nosync")){
        return;
    }
}

/* Unlocks the locks depending on the syncstrategy */
void checkUNLOCK(char *sync){
    if(strcmp(sync,"mutex") == 0){
        if (pthread_mutex_unlock(&lock) != 0){
            fprintf(stderr,"Error: Could not unlock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(sync,"rwlock") == 0){
       if(pthread_rwlock_unlock(&lockrw) != 0){
            fprintf(stderr,"Error: Could not unlock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (strcmp(sync,"nosync")){
        return;
    }
}

/*Inserts a Command in inputCommands */
int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

/* Removes Command after usage, also protected with mutex if needed */
char* removeCommand(char *argv4) {
    
    if (strcmp(argv4,"mutex") == 0||strcmp(argv4,"rwlock") == 0){
        pthread_mutex_lock(&inputLock);
        if(numberCommands > 0){
            numberCommands--;
            pthread_mutex_unlock(&inputLock);
            return inputCommands[headQueue++];  
        }
        pthread_mutex_unlock(&inputLock);
        return NULL;
    }
    else{
        if(numberCommands > 0){
            numberCommands--;
            return inputCommands[headQueue++];  
        }
        return NULL;
    }
}
/* Error Function */
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* Checks for errors in input */
void verifyInput(int argc, char* argv[]){
    numberThreads = atoi(argv[3]);
    if(argc != 5){
        fprintf(stderr, "Error: Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    else if(numberThreads <= 0){ /*Verificacao do quarto argumento */
        fprintf(stderr,"Error: Fourth argument is not valid.\n");
        exit(EXIT_FAILURE);
    }
    else if(strcmp(argv[4],"mutex") != 0 && strcmp(argv[4],"rwlock") != 0 && strcmp(argv[4],"nosync") != 0){
        fprintf(stderr,"Error: Fifth argument is not valid.\n");
        exit(EXIT_FAILURE);
    }
    else if(strcmp(argv[4],"nosync") == 0 && numberThreads != 1){
        fprintf(stderr,"Error: nosync number of Threads must be one.\n");
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
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(input);
}

/* Applies all the commands */
void *applyCommands(void *argv4){
    char *synctype = (char *)argv4;
    while (numberCommands > 0){
        const char* command = removeCommand(synctype);
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        checkLOCK(synctype,write);
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        checkUNLOCK(synctype);
                        break;
                    case 'd':
                        checkLOCK(synctype,write);
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        checkUNLOCK(synctype);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                checkLOCK(synctype,read);
                searchResult = lookup(name);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                checkUNLOCK(synctype);
                break;
            case 'd':
                checkLOCK(synctype,write);
                printf("Delete: %s\n", name);
                delete(name);
                checkUNLOCK(synctype);
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
void initThread(char *argv4){
    int i;
    pthread_t tid[numberThreads];
    
    if (strcmp(argv4,"mutex") == 0||strcmp(argv4,"rwlock") == 0) {
        if (pthread_mutex_init(&inputLock,NULL) != 0){
            fprintf(stderr,"Error: Could not init mutex inputlock.\n");
            exit(EXIT_FAILURE);
        };
        if(strcmp(argv4,"mutex") == 0){
            if(pthread_mutex_init(&lock,NULL) != 0){
                fprintf(stderr,"Error: Could not init mutex lock.\n");
                exit(EXIT_FAILURE);
            };
        }
        else if(strcmp(argv4,"rwlock") == 0){
           if(pthread_rwlock_init(&lockrw,NULL) != 0){
               fprintf(stderr,"Error: Could not init rwlock.\n");
               exit(EXIT_FAILURE);
           };
        }
    }
    gettimeofday(&t1, NULL);
    for (i = 0; i < numberThreads; i++){
        if(pthread_create (&tid[i], 0, applyCommands,argv4) != 0) 
            fprintf(stderr,"Could not create thread.\n");
    }
    for (i=0; i<numberThreads; i++){
        if(pthread_join (tid[i], NULL) != 0)
            fprintf(stderr,"Could not join thread.\n");
    }
    return;
}

/* Destroys the threads created */
void destroyThread(char *argv4){
    if (strcmp(argv4,"mutex") == 0||strcmp(argv4,"rwlock") == 0) {
        pthread_mutex_destroy(&inputLock);
        if(strcmp(argv4,"mutex") == 0){
            pthread_mutex_destroy(&lock);
        }
        else if(strcmp(argv4,"rwlock") == 0){
            pthread_rwlock_destroy(&lockrw);
        }
    }
}




int main(int argc, char* argv[]) {
   
    verifyInput(argc,argv);
   
    /* init filesystem */
    init_fs();
    processInput(argc,argv);
    
    initThread(argv[4]);
    
    /*print tree */
    print_tecnicofs_tree(argc,argv);

    /* release allocated memory */
    destroyThread(argv[4]);
    destroy_fs();
    
    gettimeofday(&t2, NULL);
    getTime(t1,t2);
    
    exit(EXIT_SUCCESS);
} 
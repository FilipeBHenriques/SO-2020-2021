#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"
#include <sys/time.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


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

socklen_t addrlen;
int sockfd;

/*Locks critical seccions depending on the syncstrategy */
void checkLOCK(){
    if (pthread_mutex_lock(&lock) != 0){
            fprintf(stderr,"Error: Could not lock.\n");
            exit(EXIT_FAILURE);
    }
}

/* Unlocks the locks depending on the syncstrategy */
void checkUNLOCK(){
    if (pthread_mutex_unlock(&lock) != 0){
        fprintf(stderr,"Error: Could not unlock.\n");
        exit(EXIT_FAILURE);
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
char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}
/* Error Function */
void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}




/* Applies all the commands */
void *applyCommands(){
    struct sockaddr_un client_addr;
    char in_buffer[MAX_INPUT_SIZE]; 
    int c;
    int i;
    while (1){
        

        addrlen=sizeof(struct sockaddr_un);
    
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0,(struct sockaddr *)&client_addr, &addrlen);
        /* Como validar */

        if (c < 0) {
            perror("server: sendto error");
            exit(EXIT_FAILURE);
        }
            //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        char token;
        char name[MAX_INPUT_SIZE],arg2[MAX_INPUT_SIZE];
        int numTokens = sscanf(in_buffer,"%c %s %s",&token,name,arg2);

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        
        switch (token) {
            case 'c':
                switch (arg2[0]) {
                    case 'f':
                        checkLOCK();
                        printf("Create file: %s\n", name);
                        i = create(name, T_FILE);
                        checkUNLOCK();
                        break;
                    case 'd':
                        checkLOCK();
                        printf("Create directory: %s\n", name);
                        i = create(name, T_DIRECTORY);
                        checkUNLOCK();
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                checkLOCK();
                i = lookup(name);
                if (i >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                checkUNLOCK();
                break;
            case 'd':
                checkLOCK();
                printf("Delete: %s\n", name);
                i = delete(name);
                checkUNLOCK();
                break;

            case 'm':
                printf("Move: %s to : %s\n", name,arg2);
                checkLOCK();
                i = move(name,arg2); 
                checkUNLOCK();
                break;
            case 'p':
                checkLOCK();
                printf("Print tree to file: %s\n",name);
                i = print_tecnicofs_tree(name);
                checkUNLOCK();
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
        if (sendto(sockfd, (&i), sizeof(i), 0, (struct sockaddr *)&client_addr, addrlen) < 0){
                perror("server: sendto error");
                exit(EXIT_FAILURE);
        }
    
    }
}


/* Initializes Threads and creates them;
   Starts the clock */
void initThread(){
    int i;
    pthread_t tid[numberThreads];

    if (pthread_mutex_init(&inputLock, NULL) != 0){
        fprintf(stderr, "Error: Could not init mutex inputlock.\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&lock, NULL) != 0){
        fprintf(stderr, "Error: Could not init mutex lock.\n");
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < numberThreads; i++){
        if(pthread_create (&tid[i], 0, applyCommands,NULL) != 0) 
            fprintf(stderr,"Could not create thread.\n");
    }
    for (i=0; i<numberThreads; i++){
        if(pthread_join (tid[i], NULL) != 0)
            fprintf(stderr,"Could not join thread.\n");
    }
    return;
}



int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}



int main(int argc, char* argv[]) {
  
  struct sockaddr_un server_addr;
  
  char *path;
    numberThreads = atoi(argv[1]);
  /* init filesystem */
  init_fs();
  
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
    perror("server: can't open socket");
    exit(EXIT_FAILURE);
  }

  path = argv[2];

  unlink(path);

  addrlen = setSockAddrUn (argv[2], &server_addr);
  if (bind(sockfd, (struct sockaddr *) &server_addr, addrlen) < 0) {
    perror("server: bind error");
    exit(EXIT_FAILURE);
  } 
  initThread();
    
    
} 
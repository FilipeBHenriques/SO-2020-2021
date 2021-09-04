#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define Max_Path 512 

int sockfd;
struct sockaddr_un serv_addr;
socklen_t servlen; 
char *servername;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
  
  char arg[MAX_INPUT_SIZE];
  int i;
  
  sprintf(arg,"c %s %c",filename,nodeType);
  if (sendto(sockfd, arg, strlen(arg)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    return -1; 
  } 

  if (recvfrom(sockfd, &i, sizeof(&i), 0, 0, 0) < 0) {
   return -1;
  } 
 
  return i;
}

int tfsDelete(char *path) {
  char arg[MAX_INPUT_SIZE];
  int i;
  
  sprintf(arg,"d %s",path);
  if (sendto(sockfd, arg, strlen(arg)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    return -1; 
  } 

  if (recvfrom(sockfd, &i, sizeof(&i), 0, 0, 0) < 0) {
   return -1;
  } 
 
  return i;
}

int tfsMove(char *from, char *to) {
  char arg[MAX_INPUT_SIZE];
  int i;
  
  sprintf(arg,"m %s %s",from,to);
  if (sendto(sockfd, arg, strlen(arg)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    return -1; 
  } 

  if (recvfrom(sockfd, &i, sizeof(&i), 0, 0, 0) < 0) {
   return -1;
  } 
 
  return i;
}

int tfsLookup(char *path) {
  char arg[MAX_INPUT_SIZE];
  int i;
  
  sprintf(arg,"l %s",path);
  if (sendto(sockfd, arg, strlen(arg)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    return -1; 
  } 

  if (recvfrom(sockfd, &i, sizeof(&i), 0, 0, 0) < 0) {
   return -1;
  } 
 
  return i;
}

int tfsTree(char *outputfile){
  char arg[MAX_INPUT_SIZE];
  int i;
  
  sprintf(arg,"p %s",outputfile);
  if (sendto(sockfd, arg, strlen(arg)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    return -1; 
  } 

  if (recvfrom(sockfd, &i, sizeof(&i), 0, 0, 0) < 0) {
   return -1;
  } 
 
  return i;
}


int tfsMount(char *sockPath) {
  char path[Max_Path]; 
  socklen_t  clilen;
  struct sockaddr_un  client_addr;
  
  
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) {
    return -1;
  }
  
  sprintf(path,"/tmp/cliente%d",getpid());
  unlink(path);

  clilen = setSockAddrUn (path, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    return -1;
  }  
  servername = sockPath;
  servlen = setSockAddrUn(sockPath, &serv_addr);

  return 0;
}


int tfsUnmount() {
   char path[Max_Path];

 
  sprintf(path,"/tmp/cliente%d",getpid());
  
  close(sockfd);
 
  unlink(path);
  
  exit(EXIT_SUCCESS);
}

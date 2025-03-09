#include <stdio.h> 
#include <stdlib.h> 
#include "server.h"
#include <sys/socket.h> 
#include <assert.h> 
#include <netinet/in.h> 

int make_socket(sockfd){ 
    int sockfd; //socket file descriptor, will contain protocols 
    assert(sockfd); 
    sockfd = socket(AF_INET, SOCK_STREAM, 443); //socket is on ipv4 protocol, uses TCP 
    assert (sockfd != -1); //if sockfd = -1 then its creation is unsucessful 
    return sockfd; 
}




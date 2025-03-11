//handles main server loop, socket creation, and accepting creatiosn 

#include <stdio.h> 
#include <stdlib.h> 
#include "server.h"
#include <sys/socket.h> 
#include <sys/types.h>
#include <assert.h> 
#include <netinet/in.h> 
#include "config.h"

int make_socket(sockfd){ 
    assert(sockfd == NULL);  //sockfd should have no initial value 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); //socket file descriptor, will contain protocols 
    //socket is on ipv4 protocol, uses TCP and TCP's default protocol (which is what 0 does). 
    assert (sockfd != -1); //if sockfd = -1 then its creation is unsucessful 
    return sockfd; 
}

int bind_socket(){
    struct sockaddr_in server_addr; 
    server_addr.sin_family = FAMILY;
    server_addr.sin_addr.s_addr = SERVER_ADDRESS;
    server_addr.sin_port = htons(PORT); 


    return 0; 
}

int listen_for_client(){
    return 0; 
}

int accept_client(){
    return 0; 
}

int close_server(){
    return 0; 
}


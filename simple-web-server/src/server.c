//handles main server loop, socket creation, and accepting creatiosn 

#include <stdio.h> 
#include <stdlib.h> 
#include "server.h"
#include <sys/socket.h> 
#include <sys/types.h>
#include <assert.h> 
#include <netinet/in.h> 
#include "config.h"

void turn_on_server(){
    int sockfd = -1; 
    make_socket(&sockfd); 
    bind_socket(sockfd);
    listen_for_client(sockfd);
    printf("Server successfully bound to port %d.\n", SERVER_PORT);
}

void make_socket(int* sockfd){ 
    assert(sockfd != NULL);   
    *sockfd = socket(AF_INET, SOCK_STREAM, 0); //socket file descriptor, will contain protocols 
    //socket is on ipv4 protocol, uses TCP and TCP's default protocol (which is what 0 does). 
    if (*sockfd == -1) {  
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
}

int bind_socket(int sockfd){
    struct sockaddr_in server_addr; 
    memset(&server_addr, 0, sizeof(server_addr)); //zeroes struct. deals with garbage values. 

    server_addr.sin_family = SERVER_FAMILY;
    server_addr.sin_addr.s_addr = htonl(SERVER_ADDRESS);
    server_addr.sin_port = htons(SERVER_PORT);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("binding failed");
        return -1;
    }

    return 0;
}

int listen_for_client(int sockfd){
    if (listen(sockfd, MAXIMUM_NUMBER_OF_PENDING_CONNECTIONS) == -1) {
        perror("listen failed");
        close(sockfd);
        return -1;
    }
    return 0;
}

int accept_client(){
    return 0; 
}

int close_server(){
    return 0; 
}


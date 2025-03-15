//handles main server loop, socket creation, and accepting creatiosn 

#include <stdio.h> 
#include <stdlib.h> 
#include "server.h"
#include <sys/socket.h> 
#include <sys/types.h>
#include <assert.h> 
#include <netinet/in.h> 
#include "config.h" 
#include <unistd.h> //for read, write, and close functions
#include <string.h> //for strlen() 

void turn_on_server(){
    int sockfd = -1; 
    make_socket(&sockfd);
    if(bind_socket(sockfd)==-1){
        perror("binding of server socket failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    } 
    if(listen_for_client(sockfd) == -1){
        perror("listening for socket failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    // Main server loop
    while (1) {
        int client_sockfd = accept_client(sockfd);
        if (client_sockfd >= 0) {
            handle_client(client_sockfd);
        }
    }
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

    server_addr.sin_family = SERVER_FAMILY; //server family can be changed in config.h
    server_addr.sin_addr.s_addr = htonl(SERVER_ADDRESS); //SERVER_ADDRESS can be changed in config.h
    server_addr.sin_port = htons(SERVER_PORT); //SERVER_PORT can be changed in config.h

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("binding failed");
        return -1;
    }
    printf("Server successfully bound to port %d.\n", SERVER_PORT);
    return 0;
}

int listen_for_client(int sockfd){
    if (listen(sockfd, MAXIMUM_NUMBER_OF_PENDING_CONNECTIONS) == -1) { //MAXIMUM_NUMBER can be changed in config.h
        perror("listen_for_client failed");
        close(sockfd);
        return -1;
    }
    return 0;
}

int accept_client(int sockfd){

    struct sockaddr_in client_addr; 
    socklen_t client_len = sizeof(client_addr);
    int new_socket; 
    new_socket = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
    if (new_socket < 0){
        perror("accept failed");
        return -1; 
    }
    printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return new_socket;
}

void handle_client(int client_sockfd) {
    char buffer[4096] = {0};
    ssize_t bytes_read = read(client_sockfd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        // parse HTTP request here
        printf("Received request:\n%s\n", buffer);
        
        // send a basic HTTP response
        const char *response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Connection: close\r\n"
                               "Content-Length: 43\r\n"
                               "\r\n"
                               "<html><body><h1>Hello, World!</h1></body></html>";
        write(client_sockfd, response, strlen(response));
    }
    close(client_sockfd);
}

int parse_http_request(){

    return 0; 
}
//creates socket. connects to the serverer. sends http requests. receives http responses.
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>  // For memset
#include <unistd.h>  // For close, read, write
#include <sys/socket.h> 
#include <sys/types.h>
#include <assert.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  // For inet_pton
#include "config.h" 
#include "client.h" // for make_socket(), connect_server(), and send_request()

void make_client(){

    int sockfd; 
    make_socket(&sockfd); 
    connect_server(sockfd);
    send_request(sockfd);
    close(sockfd);
    
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

void connect_server(int sockfd) {
    struct sockaddr_in server_addr;

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Convert IP address to binary
    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server at %s:%d\n", SERVER_IP_ADDRESS, SERVER_PORT);
}


void send_request(int sockfd) {
    char buffer[4096];
    
    // Prepare HTTP request
    const char *http_request = "GET / HTTP/1.1\r\n"
                              "Host: localhost\r\n"
                              "Connection: close\r\n"
                              "\r\n";
    
    // Send HTTP request
    if (write(sockfd, http_request, strlen(http_request)) != strlen(http_request)) {
        perror("Failed to send complete request");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    printf("HTTP request sent, awaiting response...\n");
    
    // Read and display response
    ssize_t bytes_read;
    while ((bytes_read = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate the received data
        printf("%s", buffer);
    }
    
    if (bytes_read < 0) {
        perror("Read error");
    }
}
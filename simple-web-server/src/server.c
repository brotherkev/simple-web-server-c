//handles main server loop, socket creation, and accepting creatiosn 

#include <stdio.h> 
#include <stdlib.h> 
#include "server.h" //provides Httprequest struct, and functions
#include <sys/socket.h> 
#include <sys/types.h>
#include <assert.h> 
#include <netinet/in.h> 
#include "config.h" // SERVER_ADDRESS/SERVER_FAMILY/SERVER_PORT MACROS, 
#include <unistd.h> //for read, write, and close functions
#include <string.h> //for strlen() 
#include <sys/stat.h> // for stat()
#include <fcntl.h> // for open()

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
        HttpRequest request;
        if (parse_http_request(buffer, &request) == 0) {
            printf("Method: %s, Path: %s, Version: %s\n", request.method, request.path, request.version);
            
            // Serve files based on the path
            serve_file(client_sockfd, request.path);
        } else {
            const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            write(client_sockfd, response, strlen(response));
        }
    }
    
    close(client_sockfd);
}

int parse_http_request(const char *request, HttpRequest *parsed_request){
    // example request: "GET /index.html HTTP/1.1\r\nHost: localhost\r\n..."
    char buffer[1024];
    strncpy(buffer, request, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // tokenize the request line
    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    if (!method || !path || !version) { //if no method, no path, or no version
        return -1; // invalid request
    }

    // copy parsed values into the struct
    strncpy(parsed_request->method, method, sizeof(parsed_request->method) - 1);
    strncpy(parsed_request->path, path, sizeof(parsed_request->path) - 1);
    strncpy(parsed_request->version, version, sizeof(parsed_request->version) - 1);

    return 0;  
}

void serve_file(int client_sockfd, const char *path) {
    // Default to "index.html" if the path is "/"
    char file_path[256];
    if (strcmp(path, "/") == 0) {
        strcpy(file_path, "index.html");
    } else {
        // Remove leading '/' from the path
        strncpy(file_path, path + 1, sizeof(file_path) - 1);
    }

    // Open the file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        // File not found
        const char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(client_sockfd, response, strlen(response));
        return;
    }

    // Get file size
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    // Send HTTP headers
    char headers[1024];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n", file_size);
    write(client_sockfd, headers, strlen(headers));

    // Send file content
    char file_buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
        write(client_sockfd, file_buffer, bytes_read);
    }

    close(file_fd);
}
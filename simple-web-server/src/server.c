//handles main server loop, socket creation, and accepting creatiosn 

#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <assert.h> 
#include <netinet/in.h> 
#include "config.h" // SERVER_ADDRESS/SERVER_FAMILY/SERVER_PORT MACROS, 
#include "server.h" //provides httprequest struct and other functions
#include <unistd.h> //for read, write, and close functions
#include <string.h> //for strlen() 
#include <sys/stat.h> // for stat()
#include <fcntl.h> // for open()
#include <signal.h> 


volatile sig_atomic_t shutdown_server = 0; 


void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdown_server = 1;
    }
}

void turn_on_server(){
    int sockfd = -1; 
    printf(sockfd);
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

    // set up signal handler for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // main server loop
    while (!shutdown_server) {
        int client_sockfd = accept_client(sockfd);
        if (client_sockfd >= 0) {
            handle_client(client_sockfd);
        }
    }

    printf("Shutting down server...\n");
    close(sockfd);
}


void make_socket(int* sockfd) {
    assert(sockfd != NULL); // Ensure sockfd is not NULL
    *sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket
    if (*sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE); // Exit if socket creation fails
    }
    printf("Socket created successfully: sockfd = %d\n", *sockfd); // Debug print
}

int bind_socket(int sockfd) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr)); // Zero out the struct

    // Set up the server address
    server_addr.sin_family = SERVER_FAMILY; // Use the address family from config.h
    server_addr.sin_port = htons(SERVER_PORT); // Convert port to network byte order
    
    // Convert the IP address from string to binary
    if (inet_pton(SERVER_FAMILY, SERVER_ADDRESS, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        return -1;
    }
    

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        return -1;
    }

    printf("Server successfully bound to %s:%d\n", SERVER_ADDRESS, SERVER_PORT);
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
    
    char *buffer = malloc(4096); // Dynamically allocate buffer
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        close(client_sockfd);
        return;
    }

    ssize_t bytes_read = read(client_sockfd, buffer, 4096 - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the buffer
        HttpRequest request;
        if (parse_http_request(buffer, &request) == 0) {
            printf("Method: %s, Path: %s, Version: %s\n", request.method, request.path, request.version);
            serve_file(client_sockfd, request.path);
        } else {
            const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            write(client_sockfd, response, strlen(response));
        }
    }

    free(buffer); // Free the allocated memory
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
    const char *document_root = "."; //server's document root

   //construct full server path

    if (strcmp(path, "/") == 0) {
        snprintf(file_path, sizeof(file_path), "%s/index.html", document_root);
    } else {
        snprintf(file_path, sizeof(file_path), "%s%s", document_root, path);
    }


    //file path validation; prevents traversal attacks. 
    if (strstr(file_path, "../") != NULL) {
        const char *response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        write(client_sockfd, response, strlen(response));
        return;
    }

    // Open the file
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        // File not found
        const char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(client_sockfd, response, strlen(response));
        return;
    }

    // get file size
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    // send HTTP headers
    char headers[1024];
    const char *mime_type = get_mime_type(file_path);
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n", mime_type, file_size);
    write(client_sockfd, headers, strlen(headers));

    // send file content
    char file_buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
        write(client_sockfd, file_buffer, bytes_read);
    }

    close(file_fd);
}

const char* get_mime_type(const char* path) {
    if (strstr(path, ".html")) return "text/html";
    if (strstr(path, ".css")) return "text/css";
    if (strstr(path, ".js")) return "application/javascript";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg")) return "image/jpeg";
    if (strstr(path, ".png")) return "image/png";
    if (strstr(path, ".gif")) return "image/gif";
    return "application/octet-stream";
}
//creates socket. connects to the serverer. sends http requests. receives http responses.
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //for memset
#include <unistd.h> //for close, read, write
#include <sys/socket.h>
#include <assert.h> 
#include <netinet/in.h>
#include <arpa/inet.h> //for inet_pton
#include "config.h"
#include "client.h" //for make_socket, connect_server, and send_request

void make_client() {
    int sockfd;
    make_socket(&sockfd);
    if (connect_server(sockfd) == 0) {
        send_request(sockfd);
    }
    close(sockfd);
}

void make_socket(int* sockfd) {
    assert(sockfd != NULL);
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
}

int connect_server(int sockfd) {
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
    return 0;
}

void send_request(int sockfd) {
    char *buffer = malloc(4096); // Dynamically allocate buffer
    if (!buffer) {
        perror("Failed to allocate memory for buffer");
        close(sockfd);
        return;
    }

    // Allow the user to specify the path
    char path[256];
    printf("Enter the path to request (e.g., /index.html): ");
    fgets(path, sizeof(path), stdin);
    path[strcspn(path, "\n")] = '\0'; // Remove newline character

    // Validate the path
    if (path[0] != '/') {
        fprintf(stderr, "Invalid path: Path must start with '/'\n");
        free(buffer);
        close(sockfd);
        return;
    }

    // Prepare HTTP request with keep-alive
    char http_request[1024];
    snprintf(http_request, sizeof(http_request),
             "GET %s HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "Connection: keep-alive\r\n"
             "\r\n", path);

    // Send HTTP request
    if (write(sockfd, http_request, strlen(http_request)) != strlen(http_request)) {
        perror("Failed to send complete request");
        free(buffer);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP request sent, awaiting response...\n");

    // Read and display response
    ssize_t bytes_read;
    while ((bytes_read = read(sockfd, buffer, 4096 - 1)) > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate the received data
        printf("%s", buffer);
    }

    if (bytes_read < 0) {
        perror("Read error");
    }

    free(buffer); // Free the allocated memory
}
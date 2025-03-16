#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"

void create_socket(int *sockfd) {
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
}

void connect_to_server(int sockfd) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP_ADDRESS, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP_ADDRESS, SERVER_PORT);
}

void send_request(int sockfd, const char *path) {
    char http_request[1024];
    snprintf(http_request, sizeof(http_request),
             "GET %s HTTP/1.1\r\n"
             "Host: localhost\r\n"
             "Connection: close\r\n"
             "\r\n", path);

    if (write(sockfd, http_request, strlen(http_request)) != strlen(http_request)) {
        perror("Failed to send complete request");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("HTTP request sent, awaiting response...\n");
}

void save_response(int sockfd) {
    char buffer[4096];
    FILE *temp_file = fopen("temp.html", "w");
    if (!temp_file) {
        perror("Failed to create temporary file");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read;
    while ((bytes_read = read(sockfd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        fputs(buffer, temp_file);
    }

    fclose(temp_file);
    printf("Response saved to temp.html\n");
}

void open_response_in_browser() {
    #ifdef __linux__
        system("xdg-open temp.html");
    #elif __APPLE__
        system("open temp.html");
    #elif _WIN32
        system("start temp.html");
    #else
        printf("Unsupported platform. Please open temp.html manually.\n");
    #endif
}

int main() {
    int sockfd;
    create_socket(&sockfd);
    connect_to_server(sockfd);

    char path[256];
    printf("Enter the path to request (e.g., /index.html): ");
    fgets(path, sizeof(path), stdin);
    path[strcspn(path, "\n")] = '\0';

    if (path[0] != '/') {
        fprintf(stderr, "Invalid path: Path must start with '/'\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    send_request(sockfd, path);
    save_response(sockfd);
    open_response_in_browser();

    close(sockfd);
    return 0;
}
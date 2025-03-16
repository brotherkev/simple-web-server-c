#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include "config.h"
#include "server.h"

volatile sig_atomic_t shutdown_server = 0;

void handle_signal(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        shutdown_server = 1;
    }
}

void setup_signal_handlers() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
}

int create_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void bind_socket(int sockfd) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = SERVER_FAMILY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(SERVER_FAMILY, SERVER_ADDRESS, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Server bound to %s:%d\n", SERVER_ADDRESS, SERVER_PORT);
}

void start_listening(int sockfd) {
    if (listen(sockfd, MAXIMUM_NUMBER_OF_PENDING_CONNECTIONS) == -1) {
        perror("Listening failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", SERVER_PORT);
}

int accept_connection(int sockfd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);

    if (client_sockfd < 0) {
        perror("Accept failed");
        return -1;
    }

    printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    return client_sockfd;
}

void handle_client(int client_sockfd) {
    char buffer[4096];
    ssize_t bytes_read = read(client_sockfd, buffer, sizeof(buffer) - 1);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        HttpRequest request;

        if (parse_http_request(buffer, &request) == 0) {
            printf("Method: %s, Path: %s, Version: %s\n", request.method, request.path, request.version);
            serve_file(client_sockfd, request.path);
        } else {
            const char *response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            write(client_sockfd, response, strlen(response));
        }
    }

    close(client_sockfd);
}

void serve_file(int client_sockfd, const char *path) {
    char file_path[256];
    const char *document_root = ".";
    snprintf(file_path, sizeof(file_path), "%s%s", document_root, strcmp(path, "/") == 0 ? "/index.html" : path);

    if (strstr(file_path, "../") != NULL) {
        const char *response = "HTTP/1.1 403 Forbidden\r\n\r\n";
        write(client_sockfd, response, strlen(response));
        return;
    }

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        const char *response = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(client_sockfd, response, strlen(response));
        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);

    char headers[1024];
    snprintf(headers, sizeof(headers),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n"
             "\r\n", get_mime_type(file_path), file_stat.st_size);
    write(client_sockfd, headers, strlen(headers));

    char file_buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, file_buffer, sizeof(file_buffer))) > 0) {
        write(client_sockfd, file_buffer, bytes_read);
    }

    close(file_fd);
}

void run_server() {
    int sockfd = create_socket();
    bind_socket(sockfd);
    start_listening(sockfd);
    setup_signal_handlers();

    while (!shutdown_server) {
        int client_sockfd = accept_connection(sockfd);
        if (client_sockfd >= 0) {
            handle_client(client_sockfd);
        }
    }

    printf("Shutting down server...\n");
    close(sockfd);
}

int main() {
    run_server();
    return 0;
}
//declares shared functions and constants

#ifndef SERVER_H //Header guard:
#define SERVER_H 


//function declaration, variables, and global variables go here


void turn_on_server();
void make_socket(int* sockfd);
int bind_socket(int sockfd);
int listen_for_client(int sockfd);
int accept_client(int sockfd);
void handle_client(int client_sockfd);

#endif

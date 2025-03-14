#ifndef CLIENT_H //Header guard:
#define CLIENT_H 

void make_client();
void make_socket(int* sockfd);
void connect_server(int sockfd);
void send_request(int sockfd);

#endif
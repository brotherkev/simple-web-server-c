//declares shared functions and constants

#ifndef SERVER_H //Header guard:
#define SERVER_H 


//function declaration, variables, and global variables go here

int make_socket(int a); 
int bind_socket(); 
int listen_for_client();
int accept_client(); 
int close_server(); 




#endif //end of header guard 

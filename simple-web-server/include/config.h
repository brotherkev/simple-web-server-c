//stores configurable settings like ports and root directories and ip addresses



//socket binds for server

#define SERVER_PORT 8080 //defines port number 
#define SERVER_FAMILY "AF_INET" // binds to ipv4 ip addresses
#define SERVER_ADDRESS "INADDR_ANY" //binds to any/all available ip interfaces
#define SERVER_BUFFER_SIZE 4096 //
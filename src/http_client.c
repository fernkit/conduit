#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/failures.h"

#define return_defer(value, error) do {error_message = error; result = value; goto defer;} while(0)
#define gem get_error_message
#define CHUNK_SIZE 4096

// Function to create a socket and connect to a server
int connect_to_server(const char* hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(hostname);

    int result = 0;
    const char* error_message = NULL;

    if (server == NULL) return_defer(-1, gem(ERR_HOSTNAME_RESOLUTION));
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return_defer(-1, gem(ERR_SOCKET_CREATION));
    
    server_addr.sin_family = AF_INET;    
    server_addr.sin_port = htons(port);

    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    int connect_result = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (connect_result < 0) return_defer(-1, gem(ERR_SEVER_CONNECTION));

    result = sockfd;
    sockfd = -1;
    goto defer;

    defer: 
        if(sockfd >= 0) close(sockfd);
        perror(error_message);
        return result;    
}

// Function to send an HTTP GET request
int send_http_request(int sockfd, const char* hostname, const char* path) {

    int result = 0;
    const char* error_message = NULL;

    char request[2048];
    int request_length = snprintf(request, sizeof(request), 
                             "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", 
                             path, hostname); // returns the number of bytes it would have written if there were no errors.
    if(request_length >= (int)sizeof(request) - 1) return_defer(-1, gem(ERR_BUFF_OVERFLOW));
    ssize_t bytes_sent = send(sockfd, request, strlen(request), 0);
    if(bytes_sent <= 0) return_defer(-1, gem(ERR_SEND_HTTP_REQ));

    defer:
        perror(error_message);
        return result; 
}

// Function to receive and display HTTP response
int receive_http_response(int sockfd) {
    int result = 0;
    const char* error_message = NULL;
    
    char buffer[CHUNK_SIZE]; 
    int total_bytes_recieved = 0;
    int bytes_recieved = 0;
    
    while(1){
        bytes_recieved = recv(sockfd, buffer, CHUNK_SIZE, 0);
        total_bytes_recieved += bytes_recieved;
        if (bytes_recieved > 0) {
            buffer[bytes_recieved] = '\0';
            printf("Bytes received: %d\n", bytes_recieved);
            printf("%s", buffer);
        } 
        else if (bytes_recieved == 0) {
            printf("Connection closed by server\n");
            break;
        }
        else { result = -1; perror(gem(ERR_BUFF_OVERFLOW)); break; }
    }
    
    
    return result;
}

// Main function to tie everything together
// int main(int argc, char *argv[]) {
//     const char* hostname = "jsonplaceholder.typicode.com";
//     int port = 80;
//     printf("Attempting to connect to %s on port %d...\n", hostname, port);
//     int sockfd = connect_to_server(hostname, port);
//     printf("Connection result: %d\n", sockfd);

//     if(sockfd > 0) {
//         printf("Successfully connected to server!\n");
//         if (send_http_request(sockfd, hostname, "/todos/1") < 0) {
//             printf("Failed to send HTTP request\n");
//             close(sockfd);
//             return 1;
//         }
//         if (receive_http_response(sockfd) < 0) {
//             printf("Failed to receive HTTP response\n");
//             close(sockfd);
//         }
//         close(sockfd);
//     } else {
//         printf("Failed to connect to server.\n");
//     }
//     return 0;
// }
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/failures.h"
#include <sys/time.h> 

#define return_defer(value, error) do {error_message = error; result = value; goto defer;} while(0)
#define gem get_error_message
#define CHUNK_SIZE 4096



#define GROW_BUFFER(ptr, size, needed) do {                      \
    if ((needed) > (size)) {                                     \
        size_t new_size = (size) * 2;                            \
        if (new_size < (needed)) new_size = (needed);            \
        void* new_ptr = realloc((ptr), new_size);                \
        if (!new_ptr) {                                          \
            free(ptr);                                           \
            (ptr) = NULL;                                        \
            return -1;                                           \
        }                                                        \
        (ptr) = new_ptr;                                         \
        (size) = new_size;                                       \
    }                                                            \
} while(0)


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
    size_t buffer_size = CHUNK_SIZE;
    size_t data_used = 0; 
    char* data = (char*)malloc(sizeof(char) * CHUNK_SIZE);

    if (!data) return -1;

    char temp_buffer[CHUNK_SIZE];
    ssize_t bytes_received;

    struct timeval timeout;
    timeout.tv_sec = 3; 
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int content_length = -1;
    int headers_ended = 0;
    int body_bytes_received = 0;

    while(1){
        bytes_received = recv(sockfd, buffer, CHUNK_SIZE, 0);
        total_bytes_recieved += bytes_received;

        if (bytes_received > 0) {            
            GROW_BUFFER(data, buffer_size, data_used + bytes_received + 1);
            memcpy(data + data_used, buffer, bytes_received + 1);
            data_used += bytes_received;
            data[data_used] = '\0';

            if (content_length == -1) {
                char* cl_pos = strstr(data, "Content-Length: ");
                if (cl_pos) {
                    content_length = atoi(cl_pos + 16);
                }
            }            
            if (!headers_ended) {
                char* end_headers = strstr(data, "\r\n\r\n");
                if (end_headers) {
                    headers_ended = 1;
                    body_bytes_received = data_used - ((end_headers + 4) - data);
                }
            }
            if (headers_ended && content_length != -1) {
                if (body_bytes_received >= content_length) break;
            }

            printf("Bytes received: %d\n", bytes_received);
        } 
        else if (bytes_received == 0) {
            printf("Connection closed by server\n");
            break;
        }
        else { result = -1; perror(gem(ERR_BUFF_OVERFLOW)); break; }

    }

    printf("%s", data);

    
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
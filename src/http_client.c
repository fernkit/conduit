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
#include "../include/conduit.h"

#define return_defer(value, error) do {error_message = error; result = value; has_error = 1; goto defer;} while(0)
#define gem get_error_message
#define CHUNK_SIZE 4096

#define HTTP_DEFAULT_TIMEOUT_SEC 3
#define HTTP_HEADER_CONTENT_TYPE "Content-Type: "
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length: "
#define HTTP_HEADER_DATA_TYPE "application/json;"

#define GROW_BUFFER(ptr, size, needed) do {                      \
    if ((needed) > (size)) {                                     \
        size_t new_size = (size) * 2;                            \
        if (new_size < (needed)) new_size = (needed);            \
        void* new_ptr = realloc((ptr), new_size);                \
        if (!new_ptr) {                                          \
            free(ptr);                                           \
            (ptr) = NULL;                                        \
            return NULL;                                         \
        }                                                        \
        (ptr) = new_ptr;                                         \
        (size) = new_size;                                       \
    }                                                            \
} while(0)


void log_error(const char* context, const char* message) {
    if (message != NULL) {
        fprintf(stderr, "[ERROR] %s: %s\n", context, message);
    }
}
// Function to create a socket and connect to a server
int connect_to_server(const char* hostname, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server = gethostbyname(hostname);

    int result = 0;
    const char* error_message = NULL;
    int has_error = 0;

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
        log_error("connect_to_server", error_message);
        return result;    
}

// Function to send an HTTP GET request
int send_http_request(int sockfd, const char* hostname, const char* path) {

    int result = 0;
    const char* error_message = NULL;
    int has_error = 0;

    char request[2048];
    int request_length = snprintf(request, sizeof(request), 
                             "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", 
                             path, hostname); // returns the number of bytes it would have written if there were no errors.
    if(request_length >= (int)sizeof(request) - 1) return_defer(-1, gem(ERR_BUFF_OVERFLOW));
    ssize_t bytes_sent = send(sockfd, request, strlen(request), 0);
    if(bytes_sent <= 0) return_defer(-1, gem(ERR_SEND_HTTP_REQ));

    defer:
        log_error("send_http_request", error_message);
        return result; 
}

// Function to receive and display HTTP response
ConduitResponse* receive_http_response(int sockfd) {
    int result = 0;
    const char* error_message = NULL;
    
    char buffer[CHUNK_SIZE]; 
    int total_bytes_received = 0;  // Not "recieved"
    size_t buffer_size = CHUNK_SIZE;
    size_t data_used = 0; 
    char* data = (char*)malloc(sizeof(char) * CHUNK_SIZE);

    if (!data) return NULL;

    char temp_buffer[CHUNK_SIZE];
    ssize_t bytes_received;

    struct timeval timeout;
    timeout.tv_sec = HTTP_DEFAULT_TIMEOUT_SEC; 
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        int content_length = -1;
    int headers_ended = 0;
    int body_bytes_received = 0;

    while(1){
        bytes_received = recv(sockfd, buffer, CHUNK_SIZE, 0);
        total_bytes_received += bytes_received;

        if (bytes_received > 0) {            
            GROW_BUFFER(data, buffer_size, data_used + bytes_received + 1);
            memcpy(data + data_used, buffer, bytes_received);
            data_used += bytes_received;
            data[data_used] = '\0';

            if (content_length == -1) {
                char* cl_pos = strstr(data, HTTP_HEADER_CONTENT_LENGTH);
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
        else { result = -1; log_error("receive_http_response", gem(ERR_BUFF_OVERFLOW)); break; }

    }
    ConduitResponse* response = malloc(sizeof(ConduitResponse));
    if (!response) {
        free(data);
        return NULL;
    }

    response->status_code = 0;
    response->body = NULL;
    response->headers = NULL;
    response->content_type = NULL;

    // Status Line: HTTP/1.1 200 OK
    char* status_line = data;
    char* status_end = strstr(data, "\r\n");
    if(status_end){
        char* status_start = strstr(status_line, " ");
        if (status_start) {
            response->status_code = atoi(status_start + 1);
        } 
    }

    // Headers
    char* end_headers = strstr(data, "\r\n\r\n");

    if(end_headers){
        size_t headers_length = end_headers - data;
        response->headers = malloc(headers_length + 1);
        if (response->headers) {
            memcpy(response->headers, data, headers_length);
            response->headers[headers_length] = '\0';
        }

       char* ct_start = strstr(data, HTTP_HEADER_CONTENT_TYPE);
        if (ct_start) {
            ct_start += 14;
            char* ct_end = strstr(ct_start, "\r\n");
            if (ct_end) {
                size_t ct_length = ct_end - ct_start;
                response->content_type = malloc(ct_length + 1);
                if (response->content_type) {
                    memcpy(response->content_type, ct_start, ct_length);
                    response->content_type[ct_length] = '\0';
                }
            }
        }

        // Body starts after the blank line
        char* body_start = end_headers + 4;
        size_t body_length = data_used - (body_start - data);
        response->body = malloc(body_length + 1);
        if (response->body) {
            memcpy(response->body, body_start, body_length);
            response->body[body_length] = '\0';
        }

        if (response->body && response->content_type && 
        strstr(response->content_type, HTTP_HEADER_DATA_TYPE) == response->content_type) {
            response->json = conduit_parse_json(response->body);
        } else {
            response->json = NULL;
        }

        if (response->json) {
            printf("JSON parsing successful\n");
        } else {
            printf("JSON parsing failed\n");
        }

        free(data);
    }

    
    return response;
}

void conduit_free_response(ConduitResponse* response) {
    if (!response) return;
    free(response->headers);
    free(response->body);
    free(response->content_type);
    if (response->json) json_free_value(response->json);
    free(response);
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
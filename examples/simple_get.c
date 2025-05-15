#include <stdio.h>
#include "../include/conduit.h"

int main(int argc, char* argv[]) {
    const char* hostname = "jsonplaceholder.typicode.com";
    int port = 80;
    
    printf("Connecting to %s on port %d...\n", hostname, port);
    
    int sockfd = conduit_connect(hostname, port);
    if (sockfd < 0) {
        printf("Connection failed\n");
        return 1;
    }
    
    printf("Connected successfully!\n");    
    
    if (conduit_send_request(sockfd, hostname, "/todos/1") < 0) {
        printf("Failed to send request\n");
        return 1;
    }
    
    printf("Success\n");
    
    ConduitResponse* response = conduit_receive_response(sockfd);
    if (!response) {
        printf("Failed to get response\n");
        return 1;
    }
    
    printf("Status code: %d\n", response->status_code);
    if (response->body) {
        printf("Body: %s\n", response->body);
    }
    
    if (response->headers) free(response->headers);
    if (response->body) free(response->body);
    if (response->content_type) free(response->content_type);
    free(response);
    
    return 0;
}
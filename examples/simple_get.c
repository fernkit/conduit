#include <stdio.h>
#include "../include/conduit.h"

int main(int argc, char* argv[]) {
    const char* hostname = "jsonplaceholder.typicode.com";
    int port = 80;
    
    printf("Connecting to %s on port %d...\n", hostname, port);
    
    int sockfd = conduit_connect(hostname, port);
    if (sockfd < 0) {
        printf("Connection failed");
        return 1;
    }
    
    printf("Connected successfully!\n");    
    
    // Send request
    if (conduit_send_request(sockfd, hostname, "/todos/1") < 0) {
        printf("Failed to send request\n");
        return 1;
    }
    
    // Receive response
    conduit_receive_response(sockfd);
    
    return 0;
}
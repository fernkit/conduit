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
        printf("Failed to receive response\n");
        return 1;
    }
    if (response->json && response->json->type == JSON_OBJECT) {
        JsonObject* obj = response->json->value.object;
        
        int userId = json_get_int(obj, "userId");
        int id = json_get_int(obj, "id");
        const char* title = json_get_string(obj, "title");
        int completed = json_get_bool(obj, "completed");
        
        printf("User ID: %d\n", userId);
        printf("ID: %d\n", id);
        printf("Title: %s\n", title);
        printf("Completed: %s\n", completed ? "true" : "false");
    }

    conduit_free_response(response);
        
    return 0;
}
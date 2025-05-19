#include <stdio.h>
#include <conduit.h>

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
    
    const char* json_body = "{"
        "\"title\": \"Conduit POST Example\","
        "\"body\": \"This is a test post created with the Conduit HTTP client library\","
        "\"userId\": 1"
    "}";
    
    printf("Sending POST request...\n");
    if (conduit_post_json(sockfd, hostname, "/posts", json_body) < 0) {
        printf("Failed to send POST request\n");
        return 1;
    }
    
    printf("POST request sent successfully\n");
    
    ConduitResponse* response = conduit_receive_response(sockfd);
    if (!response) {
        printf("Failed to receive response\n");
        return 1;
    }
    
    printf("Status code: %d\n", response->status_code);
    
    if (response->json && response->json->type == JSON_OBJECT) {
        JsonObject* obj = response->json->value.object;
        
        int id = json_get_int(obj, "id");
        const char* title = json_get_string(obj, "title");
        const char* body = json_get_string(obj, "body");
        int userId = json_get_int(obj, "userId");
        
        printf("\nCreated Post:\n");
        printf("ID: %d\n", id);
        printf("Title: %s\n", title);
        printf("Body: %s\n", body);
        printf("User ID: %d\n", userId);
    } else {
        printf("No JSON data in response or parsing failed\n");
        if (response->body) {
            printf("Response body: %s\n", response->body);
        }
    }
    
    // Clean up
    conduit_free_response(response);
    
    return 0;
}
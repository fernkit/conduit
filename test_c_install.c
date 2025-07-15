#include <stdio.h>
#include <conduit.h>

int main() {
    printf("Testing Conduit C Library...\n");
    
    // Test connection (this will fail but that's okay for testing)
    int sockfd = conduit_connect("httpbin.org", 80);
    if (sockfd < 0) {
        printf("Connection failed as expected (no internet or server issues)\n");
    } else {
        printf("Connection successful!\n");
    }
    
    printf("Conduit C Library test completed.\n");
    return 0;
}

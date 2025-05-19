#include "../include/conduit.h"
#include "../include/http_client.h"

int conduit_connect(const char* hostname, int port) {
    return connect_to_server(hostname, port);
}

int conduit_send_request(int sockfd, const char* hostname, const char* path) {
    return send_http_request(sockfd, hostname, path);
}

ConduitResponse* conduit_receive_response(int sockfd) {
    return receive_http_response(sockfd);
}

int conduit_post_json(int sockfd, const char* hostname, const char* path, const char* json_body) {
    return send_json_post_request(sockfd, hostname, path, json_body);
}
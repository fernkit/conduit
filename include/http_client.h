#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

int connect_to_server(const char* hostname, int port);
int send_http_request(int sockfd, const char* hostname, const char* path);
int receive_http_response(int sockfd);

#endif
#ifndef CONDUIT_HTTP_H
#define CONDUIT_HTTP_H

/**
 * @file conduit.h
 * @brief A simple HTTP client library for C
 *
 * Conduit provides functionality to connect to web servers,
 * send HTTP requests, and receive HTTP responses.
 */

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "json_parser.h"


typedef struct {
    int status_code;     
    char* body;        
    char* headers;    
    char* content_type;
    JsonValue* json;
} ConduitResponse;

/**
 * @brief Connect to a server using a hostname and port
 *
 * @param hostname The hostname to connect to (e.g., "example.com")
 * @param port The port number (e.g., 80 for HTTP, 443 for HTTPS)
 * @return Socket file descriptor if successful, negative error code otherwise
 */
int conduit_connect(const char* hostname, int port);

/**
 * @brief Send an HTTP GET request to a server
 *
 * @param sockfd Socket file descriptor from conduit_connect
 * @param hostname The hostname for the Host header
 * @param path The path for the request URL (e.g., "/api/data")
 * @return 0 if successful, negative error code otherwise
 */
int conduit_send_request(int sockfd, const char* hostname, const char* path);

/**
 * @brief Receive and process an HTTP response
 *
 * @param sockfd Socket file descriptor from conduit_connect
 * @return 0 if successful, negative error code otherwise
 */
ConduitResponse* conduit_receive_response(int sockfd);

/**
 * @brief Get a string description for an error code
 *
 * @param error The error code returned from conduit functions
 * @return A string describing the error
 */

#ifdef __cplusplus
}
#endif

#endif /* CONDUIT_HTTP_H */
#ifndef CONDUIT_C_COMPAT_H
#define CONDUIT_C_COMPAT_H

/**
 * @file conduit_c_compat.h
 * @brief C compatibility wrapper for the C++ Conduit library
 * 
 * This header provides a C-compatible API that wraps the modern C++ implementation,
 * allowing existing C code to use the new C++ backend while maintaining
 * binary compatibility.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief JSON value types (compatible with original C library)
 */
typedef enum {
    JSON_NULL = 0,
    JSON_BOOLEAN = 1,
    JSON_NUMBER = 2,
    JSON_STRING = 3,
    JSON_ARRAY = 4,
    JSON_OBJECT = 5
} JsonType;

/**
 * @brief Forward declarations for compatibility
 */
typedef struct JsonValue JsonValue;
typedef struct JsonObject JsonObject;

/**
 * @brief HTTP response structure (compatible with original C library)
 */
typedef struct {
    int status_code;
    char* body;
    char* headers;
    char* content_type;
    JsonValue* json;
} ConduitResponse;

/**
 * @brief Original C API functions - powered by C++ backend
 */

/**
 * @brief Connect to a server using hostname and port
 * @param hostname The hostname to connect to
 * @param port The port number
 * @return Socket file descriptor if successful, negative error code otherwise
 */
int conduit_connect(const char* hostname, int port);

/**
 * @brief Send an HTTP GET request
 * @param sockfd Socket file descriptor from conduit_connect
 * @param hostname The hostname for the Host header
 * @param path The path for the request URL
 * @return 0 if successful, negative error code otherwise
 */
int conduit_send_request(int sockfd, const char* hostname, const char* path);

/**
 * @brief Receive and process an HTTP response
 * @param sockfd Socket file descriptor from conduit_connect
 * @return ConduitResponse pointer if successful, NULL otherwise
 */
ConduitResponse* conduit_receive_response(int sockfd);

/**
 * @brief Send an HTTP POST request with JSON body
 * @param sockfd Socket file descriptor from conduit_connect
 * @param hostname The server hostname
 * @param path The request path
 * @param json_body The JSON request body as a string
 * @return 0 on success, negative value on error
 */
int conduit_post_json(int sockfd, const char* hostname, const char* path, const char* json_body);

/**
 * @brief Free a ConduitResponse structure
 * @param response The response to free
 */
void conduit_free_response(ConduitResponse* response);

/**
 * @brief Parse JSON string into JsonValue structure
 * @param json_string The JSON string to parse
 * @return JsonValue pointer if successful, NULL otherwise
 */
JsonValue* conduit_parse_json(const char* json_string);

/**
 * @brief Free a JsonValue structure
 * @param value The JsonValue to free
 */
void json_free_value(JsonValue* value);

/**
 * @brief Free a JsonObject structure
 * @param obj The JsonObject to free
 */
void json_free_object(JsonObject* obj);

/**
 * @brief Get integer value from JSON object
 * @param obj The JSON object
 * @param key The key to look up
 * @return Integer value, or 0 if not found
 */
int json_get_int(JsonObject* obj, const char* key);

/**
 * @brief Get string value from JSON object
 * @param obj The JSON object
 * @param key The key to look up
 * @return String value, or NULL if not found
 */
const char* json_get_string(JsonObject* obj, const char* key);

/**
 * @brief Get boolean value from JSON object
 * @param obj The JSON object
 * @param key The key to look up
 * @return Boolean value (1 for true, 0 for false)
 */
int json_get_bool(JsonObject* obj, const char* key);

#ifdef __cplusplus
}
#endif

#endif // CONDUIT_C_COMPAT_H

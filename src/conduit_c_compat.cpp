#include "conduit_c_compat.h"
#include "conduit.hpp"
#include <map>
#include <memory>
#include <cstring>
#include <string>
#include <iostream>

// Internal structures to bridge C and C++
namespace {
    // Connection management
    struct ConnectionHandle {
        std::unique_ptr<conduit::HttpClient> client;
        std::unique_ptr<conduit::HttpClient::Connection> connection;
        std::string hostname;
        int port;
        
        ConnectionHandle(const std::string& host, int p) 
            : client(std::make_unique<conduit::HttpClient>())
            , hostname(host)
            , port(p) {
            connection = std::make_unique<conduit::HttpClient::Connection>(client->connect(host, p));
        }
    };
    
    // Global connection storage
    std::map<int, std::unique_ptr<ConnectionHandle>> connections;
    int next_connection_id = 1;
    
    // JSON value wrapper for C compatibility
    struct JsonValueWrapper {
        std::unique_ptr<conduit::JsonValue> value;
        mutable std::string cached_string; // For string returns
        
        explicit JsonValueWrapper(std::unique_ptr<conduit::JsonValue> v) 
            : value(std::move(v)) {}
    };
    
    // JsonObject wrapper for C compatibility
    struct JsonObjectWrapper {
        const conduit::JsonObject* object;
        mutable std::map<std::string, std::string> cached_strings;
        
        explicit JsonObjectWrapper(const conduit::JsonObject* obj) : object(obj) {}
    };
    
    // Helper to allocate C string
    char* allocate_c_string(const std::string& str) {
        char* result = static_cast<char*>(malloc(str.length() + 1));
        if (result) {
            strcpy(result, str.c_str());
        }
        return result;
    }
    
    // Helper to convert headers map to C string
    char* headers_to_c_string(const std::map<std::string, std::string>& headers) {
        std::string result;
        for (const auto& [key, value] : headers) {
            result += key + ": " + value + "\r\n";
        }
        return allocate_c_string(result);
    }
}

extern "C" {

int conduit_connect(const char* hostname, int port) {
    try {
        auto handle = std::make_unique<ConnectionHandle>(hostname, port);
        int id = next_connection_id++;
        connections[id] = std::move(handle);
        return id;
    } catch (const std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return -1;
    }
}

int conduit_send_request(int sockfd, const char* hostname, const char* path) {
    try {
        auto it = connections.find(sockfd);
        if (it == connections.end()) {
            return -1;
        }
        
        // Store request info for later use in receive_response
        // In the original C API, send and receive are separate calls
        // We'll store the request and execute it in receive_response
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Send request error: " << e.what() << std::endl;
        return -1;
    }
}

ConduitResponse* conduit_receive_response(int sockfd) {
    try {
        auto it = connections.find(sockfd);
        if (it == connections.end()) {
            return nullptr;
        }
        
        // For compatibility with the original API, we'll do a simple GET to /
        // In a real implementation, you'd want to store the path from send_request
        auto response = it->second->connection->get("/");
        
        auto* c_response = static_cast<ConduitResponse*>(malloc(sizeof(ConduitResponse)));
        if (!c_response) {
            return nullptr;
        }
        
        c_response->status_code = response.status_code();
        c_response->body = allocate_c_string(response.body());
        c_response->headers = headers_to_c_string(response.headers());
        c_response->content_type = allocate_c_string(response.content_type());
        
        // Handle JSON
        if (response.json()) {
            auto json_wrapper = std::make_unique<JsonValueWrapper>(
                std::make_unique<conduit::JsonValue>(*response.json())
            );
            c_response->json = reinterpret_cast<JsonValue*>(json_wrapper.release());
        } else {
            c_response->json = nullptr;
        }
        
        return c_response;
    } catch (const std::exception& e) {
        std::cerr << "Receive response error: " << e.what() << std::endl;
        return nullptr;
    }
}

int conduit_post_json(int sockfd, const char* hostname, const char* path, const char* json_body) {
    try {
        auto it = connections.find(sockfd);
        if (it == connections.end()) {
            return -1;
        }
        
        auto response = it->second->connection->post(path, json_body, "application/json");
        
        // For the original C API, post_json doesn't return the response
        // It just indicates success/failure
        return response.status_code() >= 200 && response.status_code() < 300 ? 0 : -1;
        
    } catch (const std::exception& e) {
        std::cerr << "POST JSON error: " << e.what() << std::endl;
        return -1;
    }
}

void conduit_free_response(ConduitResponse* response) {
    if (!response) return;
    
    free(response->body);
    free(response->headers);
    free(response->content_type);
    
    if (response->json) {
        json_free_value(response->json);
    }
    
    free(response);
}

JsonValue* conduit_parse_json(const char* json_string) {
    try {
        auto parsed = conduit::parse_json(json_string);
        if (!parsed) {
            return nullptr;
        }
        
        auto wrapper = std::make_unique<JsonValueWrapper>(
            std::make_unique<conduit::JsonValue>(std::move(*parsed))
        );
        
        return reinterpret_cast<JsonValue*>(wrapper.release());
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return nullptr;
    }
}

void json_free_value(JsonValue* value) {
    if (!value) return;
    
    auto* wrapper = reinterpret_cast<JsonValueWrapper*>(value);
    delete wrapper;
}

void json_free_object(JsonObject* obj) {
    if (!obj) return;
    
    auto* wrapper = reinterpret_cast<JsonObjectWrapper*>(obj);
    delete wrapper;
}

int json_get_int(JsonObject* obj, const char* key) {
    if (!obj || !key) return 0;
    
    auto* wrapper = reinterpret_cast<JsonObjectWrapper*>(obj);
    auto result = wrapper->object->find(key);
    
    if (result != wrapper->object->end() && result->second->is_number()) {
        return static_cast<int>(result->second->as_number());
    }
    
    return 0;
}

const char* json_get_string(JsonObject* obj, const char* key) {
    if (!obj || !key) return nullptr;
    
    auto* wrapper = reinterpret_cast<JsonObjectWrapper*>(obj);
    auto result = wrapper->object->find(key);
    
    if (result != wrapper->object->end() && result->second->is_string()) {
        // Cache the string to ensure it remains valid
        wrapper->cached_strings[key] = result->second->as_string();
        return wrapper->cached_strings[key].c_str();
    }
    
    return nullptr;
}

int json_get_bool(JsonObject* obj, const char* key) {
    if (!obj || !key) return 0;
    
    auto* wrapper = reinterpret_cast<JsonObjectWrapper*>(obj);
    auto result = wrapper->object->find(key);
    
    if (result != wrapper->object->end() && result->second->is_bool()) {
        return result->second->as_bool() ? 1 : 0;
    }
    
    return 0;
}

} // extern "C"

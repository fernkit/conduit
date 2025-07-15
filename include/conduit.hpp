#ifndef CONDUIT_HPP
#define CONDUIT_HPP

/**
 * @file conduit.hpp
 * @brief Modern C++ HTTP client library
 * 
 * Conduit C++ provides a modern, RAII-based HTTP client library with
 * automatic resource management, exception handling, and STL integration.
 */

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <chrono>
#include <optional>
#include <variant>

namespace conduit {

/**
 * @brief JSON value types supported by the library
 */
enum class JsonType {
    Null,
    Boolean,
    Number,
    String,
    Array,
    Object
};

/**
 * @brief Simple JSON value class with shared ownership
 */
class JsonValue {
public:
    JsonValue() : type_(JsonType::Null) {}
    explicit JsonValue(bool val) : type_(JsonType::Boolean), bool_value_(val) {}
    explicit JsonValue(double val) : type_(JsonType::Number), number_value_(val) {}
    explicit JsonValue(const std::string& val) : type_(JsonType::String), string_value_(val) {}
    
    JsonType type() const { return type_; }
    
    bool is_null() const { return type_ == JsonType::Null; }
    bool is_bool() const { return type_ == JsonType::Boolean; }
    bool is_number() const { return type_ == JsonType::Number; }
    bool is_string() const { return type_ == JsonType::String; }
    bool is_array() const { return type_ == JsonType::Array; }
    bool is_object() const { return type_ == JsonType::Object; }

    // Convenience accessors
    bool as_bool() const { return bool_value_; }
    double as_number() const { return number_value_; }
    const std::string& as_string() const { return string_value_; }
    
    // For arrays and objects, we'll use shared_ptr for simplicity
    void set_array(std::shared_ptr<std::vector<std::shared_ptr<JsonValue>>> array) {
        type_ = JsonType::Array;
        array_value_ = array;
    }
    
    void set_object(std::shared_ptr<std::map<std::string, std::shared_ptr<JsonValue>>> object) {
        type_ = JsonType::Object;
        object_value_ = object;
    }
    
    std::shared_ptr<std::vector<std::shared_ptr<JsonValue>>> as_array() const {
        return array_value_;
    }
    
    std::shared_ptr<std::map<std::string, std::shared_ptr<JsonValue>>> as_object() const {
        return object_value_;
    }

    // Object access helpers
    std::optional<int> get_int(const std::string& key) const;
    std::optional<std::string> get_string(const std::string& key) const;
    std::optional<bool> get_bool(const std::string& key) const;
    std::optional<double> get_number(const std::string& key) const;

private:
    JsonType type_;
    bool bool_value_ = false;
    double number_value_ = 0.0;
    std::string string_value_;
    std::shared_ptr<std::vector<std::shared_ptr<JsonValue>>> array_value_;
    std::shared_ptr<std::map<std::string, std::shared_ptr<JsonValue>>> object_value_;
};

/**
 * @brief JSON parsing functions (forward declared)
 */
std::optional<JsonValue> parse_json(const std::string& json_string);
std::string serialize_json(const JsonValue& value);

/**
 * @brief HTTP response representation
 */
class Response {
public:
    Response(int status_code, std::string body, std::map<std::string, std::string> headers)
        : status_code_(status_code), body_(std::move(body)), headers_(std::move(headers)) {
        
        // Parse JSON if content type indicates JSON
        auto content_type = get_header("Content-Type");
        if (content_type && content_type->find("application/json") != std::string::npos) {
            json_ = parse_json(body_);
        }
    }

    int status_code() const { return status_code_; }
    const std::string& body() const { return body_; }
    const std::map<std::string, std::string>& headers() const { return headers_; }
    const std::optional<JsonValue>& json() const { return json_; }

    std::optional<std::string> get_header(const std::string& name) const {
        auto it = headers_.find(name);
        return it != headers_.end() ? std::optional<std::string>(it->second) : std::nullopt;
    }

    std::string content_type() const {
        auto ct = get_header("Content-Type");
        return ct ? *ct : "";
    }

private:
    int status_code_;
    std::string body_;
    std::map<std::string, std::string> headers_;
    std::optional<JsonValue> json_;
};

/**
 * @brief HTTP client exception types
 */
class HttpException : public std::exception {
public:
    explicit HttpException(const std::string& message) : message_(message) {}
    const char* what() const noexcept override { return message_.c_str(); }

private:
    std::string message_;
};

class ConnectionException : public HttpException {
public:
    explicit ConnectionException(const std::string& message) : HttpException("Connection error: " + message) {}
};

class RequestException : public HttpException {
public:
    explicit RequestException(const std::string& message) : HttpException("Request error: " + message) {}
};

class ResponseException : public HttpException {
public:
    explicit ResponseException(const std::string& message) : HttpException("Response error: " + message) {}
};

/**
 * @brief HTTP client configuration
 */
struct ClientConfig {
    std::chrono::seconds timeout{30};
    std::map<std::string, std::string> default_headers;
    bool verify_ssl{true};
    std::optional<std::string> user_agent;
    
    ClientConfig() {
        default_headers["User-Agent"] = "Conduit-CPP/1.0";
    }
};

/**
 * @brief Main HTTP client class
 */
class HttpClient {
public:
    explicit HttpClient(const ClientConfig& config = ClientConfig{});
    ~HttpClient();

    // Non-copyable but movable
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&&) = default;
    HttpClient& operator=(HttpClient&&) = default;

    /**
     * @brief Connection class for persistent connections
     */
    class Connection {
    public:
        Connection(const std::string& hostname, int port, const ClientConfig& config);
        ~Connection();
        
        // Non-copyable but movable
        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;
        Connection(Connection&&) = default;
        Connection& operator=(Connection&&) = default;

        Response get(const std::string& path, const std::map<std::string, std::string>& headers = {});
        Response post(const std::string& path, const std::string& body, 
                     const std::string& content_type = "application/json",
                     const std::map<std::string, std::string>& headers = {});
        Response post_json(const std::string& path, const JsonValue& json,
                          const std::map<std::string, std::string>& headers = {});

    private:
        std::string hostname_;
        int port_;
        ClientConfig config_;
        int socket_fd_;
        bool connected_;
        
        void connect();
        void disconnect();
        Response send_request(const std::string& method, const std::string& path,
                             const std::string& body, const std::map<std::string, std::string>& headers);
    };

    /**
     * @brief Create a connection to a server
     */
    Connection connect(const std::string& hostname, int port = 80);

    /**
     * @brief Convenience methods for one-off requests
     */
    Response get(const std::string& url, const std::map<std::string, std::string>& headers = {});
    Response post(const std::string& url, const std::string& body,
                 const std::string& content_type = "application/json",
                 const std::map<std::string, std::string>& headers = {});
    Response post_json(const std::string& url, const JsonValue& json,
                      const std::map<std::string, std::string>& headers = {});

private:
    ClientConfig config_;
};

/**
 * @brief URL parsing utilities
 */
struct ParsedUrl {
    std::string scheme;
    std::string host;
    int port;
    std::string path;
    std::string query;
};

ParsedUrl parse_url(const std::string& url);

} // namespace conduit

#endif // CONDUIT_HPP

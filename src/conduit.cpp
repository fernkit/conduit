#include "conduit.hpp"
#include <iostream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <array>

// System includes for socket operations
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

namespace conduit {

namespace {
    constexpr size_t BUFFER_SIZE = 4096;
    constexpr int DEFAULT_TIMEOUT_SEC = 30;
    
    /**
     * @brief RAII socket wrapper
     */
    class Socket {
    public:
        Socket() : fd_(-1) {}
        
        explicit Socket(int fd) : fd_(fd) {}
        
        ~Socket() {
            close();
        }
        
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;
        
        Socket(Socket&& other) noexcept : fd_(other.fd_) {
            other.fd_ = -1;
        }
        
        Socket& operator=(Socket&& other) noexcept {
            if (this != &other) {
                close();
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }
        
        int fd() const { return fd_; }
        bool is_valid() const { return fd_ >= 0; }
        
        void close() {
            if (fd_ >= 0) {
                ::close(fd_);
                fd_ = -1;
            }
        }
        
        int release() {
            int fd = fd_;
            fd_ = -1;
            return fd;
        }
        
    private:
        int fd_;
    };
    
    /**
     * @brief Set socket timeout
     */
    void set_socket_timeout(int sockfd, std::chrono::seconds timeout) {
        struct timeval tv;
        tv.tv_sec = timeout.count();
        tv.tv_usec = 0;
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            throw ConnectionException("Failed to set receive timeout");
        }
        
        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
            throw ConnectionException("Failed to set send timeout");
        }
    }
    
    /**
     * @brief Create and connect socket
     */
    Socket connect_socket(const std::string& hostname, int port) {
        struct hostent* server = gethostbyname(hostname.c_str());
        if (!server) {
            throw ConnectionException("Hostname resolution failed for: " + hostname);
        }
        
        Socket sock(socket(AF_INET, SOCK_STREAM, 0));
        if (!sock.is_valid()) {
            throw ConnectionException("Socket creation failed");
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
        
        if (connect(sock.fd(), reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            throw ConnectionException("Connection failed to " + hostname + ":" + std::to_string(port));
        }
        
        return sock;
    }
    
    /**
     * @brief Send data through socket
     */
    void send_data(int sockfd, const std::string& data) {
        size_t total_sent = 0;
        while (total_sent < data.length()) {
            ssize_t sent = send(sockfd, data.c_str() + total_sent, data.length() - total_sent, 0);
            if (sent <= 0) {
                throw RequestException("Failed to send data");
            }
            total_sent += sent;
        }
    }
    
    /**
     * @brief Receive HTTP response
     */
    std::string receive_response(int sockfd) {
        std::string response;
        std::array<char, BUFFER_SIZE> buffer;
        
        int content_length = -1;
        bool headers_complete = false;
        size_t header_end_pos = 0;
        
        while (true) {
            ssize_t bytes_received = recv(sockfd, buffer.data(), buffer.size(), 0);
            
            if (bytes_received > 0) {
                response.append(buffer.data(), bytes_received);
                
                // Check for end of headers
                if (!headers_complete) {
                    size_t header_end = response.find("\r\n\r\n");
                    if (header_end != std::string::npos) {
                        headers_complete = true;
                        header_end_pos = header_end + 4;
                        
                        // Parse Content-Length
                        size_t cl_pos = response.find("Content-Length: ");
                        if (cl_pos != std::string::npos && cl_pos < header_end) {
                            size_t cl_start = cl_pos + 16;
                            size_t cl_end = response.find("\r\n", cl_start);
                            if (cl_end != std::string::npos) {
                                content_length = std::stoi(response.substr(cl_start, cl_end - cl_start));
                            }
                        }
                    }
                }
                
                // Check if we have complete response
                if (headers_complete && content_length >= 0) {
                    size_t body_length = response.length() - header_end_pos;
                    if (body_length >= static_cast<size_t>(content_length)) {
                        break;
                    }
                }
            } else if (bytes_received == 0) {
                // Connection closed
                break;
            } else {
                throw ResponseException("Failed to receive response data");
            }
        }
        
        return response;
    }
    
    /**
     * @brief Parse HTTP response
     */
    Response parse_http_response(const std::string& response_data) {
        // Find the end of headers
        size_t header_end = response_data.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            throw ResponseException("Invalid HTTP response format");
        }
        
        std::string headers_section = response_data.substr(0, header_end);
        std::string body = response_data.substr(header_end + 4);
        
        // Parse status line
        size_t first_line_end = headers_section.find("\r\n");
        if (first_line_end == std::string::npos) {
            throw ResponseException("Invalid HTTP status line");
        }
        
        std::string status_line = headers_section.substr(0, first_line_end);
        
        // Extract status code
        size_t first_space = status_line.find(' ');
        size_t second_space = status_line.find(' ', first_space + 1);
        if (first_space == std::string::npos || second_space == std::string::npos) {
            throw ResponseException("Invalid HTTP status line format");
        }
        
        int status_code = std::stoi(status_line.substr(first_space + 1, second_space - first_space - 1));
        
        // Parse headers
        std::map<std::string, std::string> headers;
        std::istringstream header_stream(headers_section.substr(first_line_end + 2));
        std::string header_line;
        
        while (std::getline(header_stream, header_line)) {
            if (header_line.back() == '\r') {
                header_line.pop_back();
            }
            
            size_t colon_pos = header_line.find(':');
            if (colon_pos != std::string::npos) {
                std::string name = header_line.substr(0, colon_pos);
                std::string value = header_line.substr(colon_pos + 1);
                
                // Trim whitespace
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                headers[name] = value;
            }
        }
        
        return Response(status_code, std::move(body), std::move(headers));
    }
    
    /**
     * @brief Build HTTP request
     */
    std::string build_http_request(const std::string& method, const std::string& path,
                                  const std::string& hostname, const std::string& body,
                                  const std::map<std::string, std::string>& headers) {
        std::ostringstream request;
        request << method << " " << path << " HTTP/1.1\r\n";
        request << "Host: " << hostname << "\r\n";
        
        // Add custom headers
        for (const auto& [name, value] : headers) {
            request << name << ": " << value << "\r\n";
        }
        
        // Add Content-Length for POST requests
        if (!body.empty()) {
            request << "Content-Length: " << body.length() << "\r\n";
        }
        
        request << "\r\n";
        
        if (!body.empty()) {
            request << body;
        }
        
        return request.str();
    }
} // anonymous namespace

// JsonValue implementations
std::optional<int> JsonValue::get_int(const std::string& key) const {
    if (!is_object()) return std::nullopt;
    
    auto obj = as_object();
    if (!obj) return std::nullopt;
    
    auto it = obj->find(key);
    if (it == obj->end()) return std::nullopt;
    
    if (it->second->is_number()) {
        return static_cast<int>(it->second->as_number());
    }
    return std::nullopt;
}

std::optional<std::string> JsonValue::get_string(const std::string& key) const {
    if (!is_object()) return std::nullopt;
    
    auto obj = as_object();
    if (!obj) return std::nullopt;
    
    auto it = obj->find(key);
    if (it == obj->end()) return std::nullopt;
    
    if (it->second->is_string()) {
        return it->second->as_string();
    }
    return std::nullopt;
}

std::optional<bool> JsonValue::get_bool(const std::string& key) const {
    if (!is_object()) return std::nullopt;
    
    auto obj = as_object();
    if (!obj) return std::nullopt;
    
    auto it = obj->find(key);
    if (it == obj->end()) return std::nullopt;
    
    if (it->second->is_bool()) {
        return it->second->as_bool();
    }
    return std::nullopt;
}

std::optional<double> JsonValue::get_number(const std::string& key) const {
    if (!is_object()) return std::nullopt;
    
    auto obj = as_object();
    if (!obj) return std::nullopt;
    
    auto it = obj->find(key);
    if (it == obj->end()) return std::nullopt;
    
    if (it->second->is_number()) {
        return it->second->as_number();
    }
    return std::nullopt;
}

// Connection implementation
HttpClient::Connection::Connection(const std::string& hostname, int port, const ClientConfig& config)
    : hostname_(hostname), port_(port), config_(config), socket_fd_(-1), connected_(false) {
    connect();
}

HttpClient::Connection::~Connection() {
    disconnect();
}

void HttpClient::Connection::connect() {
    if (connected_) return;
    
    struct hostent* server = gethostbyname(hostname_.c_str());
    if (!server) {
        throw ConnectionException("Hostname resolution failed for: " + hostname_);
    }
    
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        throw ConnectionException("Socket creation failed");
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    if (::connect(socket_fd_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        throw ConnectionException("Connection failed to " + hostname_ + ":" + std::to_string(port_));
    }
    
    set_socket_timeout(socket_fd_, config_.timeout);
    connected_ = true;
}

void HttpClient::Connection::disconnect() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    connected_ = false;
}

Response HttpClient::Connection::get(const std::string& path, const std::map<std::string, std::string>& headers) {
    return send_request("GET", path, "", headers);
}

Response HttpClient::Connection::post(const std::string& path, const std::string& body,
                                    const std::string& content_type,
                                    const std::map<std::string, std::string>& headers) {
    auto merged_headers = headers;
    merged_headers["Content-Type"] = content_type;
    return send_request("POST", path, body, merged_headers);
}

Response HttpClient::Connection::post_json(const std::string& path, const JsonValue& json,
                                         const std::map<std::string, std::string>& headers) {
    std::string json_body = serialize_json(json);
    return post(path, json_body, "application/json", headers);
}

Response HttpClient::Connection::send_request(const std::string& method, const std::string& path,
                                             const std::string& body, const std::map<std::string, std::string>& headers) {
    if (!connected_) {
        throw ConnectionException("Not connected to server");
    }
    
    // Merge default headers with request headers
    auto merged_headers = config_.default_headers;
    merged_headers.insert(headers.begin(), headers.end());
    
    std::string request = build_http_request(method, path, hostname_, body, merged_headers);
    send_data(socket_fd_, request);
    
    std::string response_data = receive_response(socket_fd_);
    return parse_http_response(response_data);
}

// HttpClient implementation
HttpClient::HttpClient(const ClientConfig& config) : config_(config) {}

HttpClient::~HttpClient() = default;

HttpClient::Connection HttpClient::connect(const std::string& hostname, int port) {
    return Connection(hostname, port, config_);
}

Response HttpClient::get(const std::string& url, const std::map<std::string, std::string>& headers) {
    ParsedUrl parsed = parse_url(url);
    Connection conn = connect(parsed.host, parsed.port);
    return conn.get(parsed.path + (parsed.query.empty() ? "" : "?" + parsed.query), headers);
}

Response HttpClient::post(const std::string& url, const std::string& body,
                         const std::string& content_type,
                         const std::map<std::string, std::string>& headers) {
    ParsedUrl parsed = parse_url(url);
    Connection conn = connect(parsed.host, parsed.port);
    return conn.post(parsed.path, body, content_type, headers);
}

Response HttpClient::post_json(const std::string& url, const JsonValue& json,
                              const std::map<std::string, std::string>& headers) {
    ParsedUrl parsed = parse_url(url);
    Connection conn = connect(parsed.host, parsed.port);
    return conn.post_json(parsed.path, json, headers);
}

ParsedUrl parse_url(const std::string& url) {
    ParsedUrl result;
    
    // Simple URL parser - more robust parsing could be added
    std::regex url_regex(R"(^(https?):\/\/([^:\/]+)(?::(\d+))?([^?]*)(?:\?(.*))?$)");
    std::smatch matches;
    
    if (std::regex_match(url, matches, url_regex)) {
        result.scheme = matches[1];
        result.host = matches[2];
        result.port = matches[3].matched ? std::stoi(matches[3]) : (result.scheme == "https" ? 443 : 80);
        result.path = matches[4].matched ? matches[4].str() : "/";
        result.query = matches[5].matched ? matches[5].str() : "";
    } else {
        throw std::invalid_argument("Invalid URL format: " + url);
    }
    
    return result;
}

} // namespace conduit

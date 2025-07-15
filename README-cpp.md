# Conduit C++ - Modern HTTP Client Library

<p align="center">
 <img src="assets/logo.png" alt="Conduit Logo" width="200"/>
</p>

A modern C++ HTTP client library with optional C compatibility layer. Built on modern C++17 features with RAII, smart pointers, and STL integration.

## Features

### C++ API
- **Modern C++17 Design**: Uses RAII, smart pointers, and STL containers
- **Exception-Safe**: Proper error handling with custom exception types
- **Type-Safe JSON**: Template-based JSON parsing with compile-time type checking
- **Connection Management**: Persistent connections with automatic cleanup
- **Header-Only Option**: Can be used as header-only library
- **STL Integration**: Works seamlessly with standard containers and algorithms

### C Compatibility Layer
- **Drop-in Replacement**: Compatible with original C API
- **Zero-Cost Abstraction**: C++ implementation with C interface
- **Memory Management**: Automatic cleanup with manual free functions
- **Same Performance**: Leverages C++ optimizations under the hood

## Requirements

- C++17 compatible compiler (gcc 7+, clang 6+, MSVC 2019+)
- CMake 3.14 or higher
- POSIX-compatible operating system (Linux, macOS, *BSD, etc.)

## Installation

### Using CMake

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
sudo make install
sudo ldconfig  # Linux only
```

### Using the Library in Your Project

#### CMake Integration
```cmake
find_package(conduit-cpp REQUIRED)
add_executable(your_app main.cpp)
target_link_libraries(your_app PRIVATE ConduitCpp::conduit-cpp)
```

#### pkg-config Integration
```bash
g++ -std=c++17 main.cpp $(pkg-config --cflags --libs conduit-cpp)
```

## C++ API Usage

### Basic HTTP Requests

```cpp
#include <conduit.hpp>
#include <iostream>

int main() {
    try {
        conduit::HttpClient client;
        
        // GET request
        auto response = client.get("http://httpbin.org/get");
        std::cout << "Status: " << response.status_code() << std::endl;
        std::cout << "Body: " << response.body() << std::endl;
        
        // POST request with JSON
        conduit::JsonObject data;
        data["name"] = std::make_unique<conduit::JsonValue>("John Doe");
        data["age"] = std::make_unique<conduit::JsonValue>(30.0);
        
        conduit::JsonValue json_body(std::move(data));
        auto post_response = client.post_json("http://httpbin.org/post", json_body);
        
        std::cout << "POST Status: " << post_response.status_code() << std::endl;
        
    } catch (const conduit::HttpException& e) {
        std::cerr << "HTTP Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Persistent Connections

```cpp
#include <conduit.hpp>

int main() {
    try {
        conduit::HttpClient client;
        
        // Create persistent connection
        auto connection = client.connect("httpbin.org", 80);
        
        // Multiple requests on same connection
        auto response1 = connection.get("/get");
        auto response2 = connection.get("/status/200");
        
        // Connection automatically closed when going out of scope
        
    } catch (const conduit::HttpException& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### JSON Handling

```cpp
#include <conduit.hpp>
#include <iostream>

int main() {
    // Parse JSON from string
    std::string json_str = R"({
        "name": "Alice",
        "age": 25,
        "active": true,
        "scores": [85, 90, 78]
    })";
    
    auto json = conduit::parse_json(json_str);
    if (json && json->is_object()) {
        auto name = json->get_string("name");
        auto age = json->get_int("age");
        auto active = json->get_bool("active");
        
        std::cout << "Name: " << (name ? *name : "N/A") << std::endl;
        std::cout << "Age: " << (age ? std::to_string(*age) : "N/A") << std::endl;
        std::cout << "Active: " << (active ? (*active ? "yes" : "no") : "N/A") << std::endl;
    }
    
    // Create and serialize JSON
    conduit::JsonObject person;
    person["name"] = std::make_unique<conduit::JsonValue>("Bob");
    person["age"] = std::make_unique<conduit::JsonValue>(35.0);
    
    conduit::JsonValue json_value(std::move(person));
    std::string serialized = conduit::serialize_json(json_value);
    std::cout << "Serialized: " << serialized << std::endl;
    
    return 0;
}
```

### Configuration and Headers

```cpp
#include <conduit.hpp>

int main() {
    // Configure client
    conduit::ClientConfig config;
    config.timeout = std::chrono::seconds(10);
    config.default_headers["User-Agent"] = "MyApp/1.0";
    config.default_headers["Accept"] = "application/json";
    
    conduit::HttpClient client(config);
    
    // Add request-specific headers
    std::map<std::string, std::string> headers;
    headers["Authorization"] = "Bearer token123";
    
    auto response = client.get("http://httpbin.org/headers", headers);
    
    return 0;
}
```

## C API Usage (Compatibility Layer)

For existing C code, you can use the compatibility layer:

```c
#include <conduit_c_compat.h>
#include <stdio.h>

int main() {
    // Connect to server
    int sockfd = conduit_connect("jsonplaceholder.typicode.com", 80);
    if (sockfd < 0) {
        printf("Connection failed\n");
        return 1;
    }
    
    // Send GET request
    if (conduit_send_request(sockfd, "jsonplaceholder.typicode.com", "/posts/1") < 0) {
        printf("Request failed\n");
        return 1;
    }
    
    // Receive response
    ConduitResponse* response = conduit_receive_response(sockfd);
    if (response) {
        printf("Status: %d\n", response->status_code);
        printf("Body: %s\n", response->body);
        
        // Parse JSON if available
        if (response->json && response->json->type == JSON_OBJECT) {
            JsonObject* obj = response->json->value.object;
            const char* title = json_get_string(obj, "title");
            int user_id = json_get_int(obj, "userId");
            
            printf("Title: %s\n", title);
            printf("User ID: %d\n", user_id);
        }
        
        conduit_free_response(response);
    }
    
    return 0;
}
```

## Exception Types

The C++ API provides specific exception types for better error handling:

```cpp
try {
    // HTTP operations
} catch (const conduit::ConnectionException& e) {
    // Handle connection errors
} catch (const conduit::RequestException& e) {
    // Handle request errors
} catch (const conduit::ResponseException& e) {
    // Handle response parsing errors
} catch (const conduit::HttpException& e) {
    // Handle any HTTP-related error
}
```

## Performance Considerations

- **Connection Reuse**: Use persistent connections for multiple requests to the same host
- **Memory Management**: C++ version uses RAII for automatic cleanup
- **JSON Parsing**: On-demand parsing - JSON is only parsed when accessed
- **String Handling**: Efficient string handling with move semantics

## Migration from C to C++

### Simple Migration
1. Change includes from `conduit.h` to `conduit.hpp`
2. Wrap calls in try-catch blocks
3. Use modern C++ features like auto and smart pointers

### Advanced Migration
1. Use `conduit::JsonValue` instead of manual JSON handling
2. Leverage STL containers for headers and data
3. Use RAII for automatic resource management
4. Take advantage of type safety and compile-time checks

## Building Examples

```bash
# Build C++ examples
mkdir build && cd build
cmake ..
make

# Run examples
./simple_example_cpp
./json_example_cpp
```

## Testing

```bash
# Run tests
cd build
make test

# Or run specific test
ctest -V
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

## License

MIT License - see LICENSE file for details

## Comparison with Original C Library

| Feature | C Library | C++ Library |
|---------|-----------|-------------|
| Memory Management | Manual | RAII |
| Error Handling | Return codes | Exceptions |
| JSON Parsing | Manual struct handling | Type-safe templates |
| String Handling | char* | std::string |
| Connection Management | Manual cleanup | Automatic |
| Header Management | Manual concatenation | std::map |
| Performance | Good | Excellent |
| Type Safety | Runtime | Compile-time |
| STL Integration | None | Full |

## Future Enhancements

- [ ] HTTPS/TLS support
- [ ] HTTP/2 support
- [ ] Async/await API
- [ ] Connection pooling
- [ ] Compression support (gzip, deflate)
- [ ] Cookie management
- [ ] Proxy support
- [ ] WebSocket support

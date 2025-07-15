#include <iostream>
#include <conduit.hpp>

int main() {
    try {
        // Create HTTP client
        conduit::HttpClient client;
        
        // Example 1: Simple GET request
        std::cout << "=== GET Request Example ===" << std::endl;
        auto response = client.get("http://jsonplaceholder.typicode.com/posts/1");
        
        std::cout << "Status Code: " << response.status_code() << std::endl;
        std::cout << "Content-Type: " << response.content_type() << std::endl;
        std::cout << "Body: " << response.body() << std::endl;
        
        // Example 2: Parse JSON response
        if (response.json()) {
            std::cout << "\n=== JSON Parsing Example ===" << std::endl;
            const auto& json = *response.json();
            
            if (json.is_object()) {
                auto title = json.get_string("title");
                auto user_id = json.get_int("userId");
                
                if (title) {
                    std::cout << "Title: " << *title << std::endl;
                }
                if (user_id) {
                    std::cout << "User ID: " << *user_id << std::endl;
                }
            }
        }
        
        // Example 3: POST request with JSON
        std::cout << "\n=== POST Request Example ===" << std::endl;
        
        // Create JSON object using the new simplified API
        auto post_data = std::make_shared<std::map<std::string, std::shared_ptr<conduit::JsonValue>>>();
        (*post_data)["title"] = std::make_shared<conduit::JsonValue>("C++ HTTP Client Test");
        (*post_data)["body"] = std::make_shared<conduit::JsonValue>("This is a test post from the C++ version");
        (*post_data)["userId"] = std::make_shared<conduit::JsonValue>(1.0);
        
        conduit::JsonValue json_body;
        json_body.set_object(post_data);
        
        auto post_response = client.post_json("http://jsonplaceholder.typicode.com/posts", json_body);
        
        std::cout << "POST Status Code: " << post_response.status_code() << std::endl;
        std::cout << "POST Response: " << post_response.body() << std::endl;
        
        // Example 4: Using persistent connections
        std::cout << "\n=== Persistent Connection Example ===" << std::endl;
        
        auto connection = client.connect("jsonplaceholder.typicode.com", 80);
        
        // Multiple requests on same connection
        auto get_response = connection.get("/posts/1");
        std::cout << "GET Status: " << get_response.status_code() << std::endl;
        
        auto post_response2 = connection.post_json("/posts", json_body);
        std::cout << "POST Status: " << post_response2.status_code() << std::endl;
        
        std::cout << "\nAll examples completed successfully!" << std::endl;
        
    } catch (const conduit::HttpException& e) {
        std::cerr << "HTTP Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

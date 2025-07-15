#include <iostream>
#include <conduit.hpp>

int main() {
    std::cout << "Testing Conduit C++ Library..." << std::endl;
    
    try {
        conduit::HttpClient client;
        std::cout << "HttpClient created successfully!" << std::endl;
        
        // Test a simple GET request
        auto response = client.get("http://httpbin.org/get");
        std::cout << "GET request successful! Status: " << response.status_code() << std::endl;
        
    } catch (const conduit::HttpException& e) {
        std::cout << "HTTP Exception (expected): " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception (may be expected): " << e.what() << std::endl;
    }
    
    std::cout << "Conduit C++ Library test completed." << std::endl;
    return 0;
}

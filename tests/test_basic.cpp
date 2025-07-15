#include <iostream>
#include <cassert>
#include <string>
#include <memory>

// We'll include the header directly for testing
#include "../include/conduit.hpp"

// Simple test functions
void test_json_parsing() {
    std::cout << "Testing JSON parsing..." << std::endl;
    
    std::string json_str = R"({
        "name": "Test User",
        "age": 25,
        "active": true
    })";
    
    auto json = conduit::parse_json(json_str);
    assert(json.has_value());
    assert(json->is_object());
    
    auto name = json->get_string("name");
    assert(name.has_value());
    assert(*name == "Test User");
    
    auto age = json->get_int("age");
    assert(age.has_value());
    assert(*age == 25);
    
    auto active = json->get_bool("active");
    assert(active.has_value());
    assert(*active == true);
    
    std::cout << "✓ JSON parsing tests passed" << std::endl;
}

void test_json_serialization() {
    std::cout << "Testing JSON serialization..." << std::endl;
    
    // Create JSON object using the new simplified API
    auto obj = std::make_shared<std::map<std::string, std::shared_ptr<conduit::JsonValue>>>();
    (*obj)["name"] = std::make_shared<conduit::JsonValue>("Alice");
    (*obj)["age"] = std::make_shared<conduit::JsonValue>(30.0);
    (*obj)["active"] = std::make_shared<conduit::JsonValue>(true);
    
    conduit::JsonValue json_val;
    json_val.set_object(obj);
    std::string serialized = conduit::serialize_json(json_val);
    
    // Should contain our values
    assert(serialized.find("Alice") != std::string::npos);
    assert(serialized.find("30") != std::string::npos);
    assert(serialized.find("true") != std::string::npos);
    
    std::cout << "✓ JSON serialization tests passed" << std::endl;
}

void test_url_parsing() {
    std::cout << "Testing URL parsing..." << std::endl;
    
    auto parsed = conduit::parse_url("http://example.com:8080/path?query=value");
    assert(parsed.scheme == "http");
    assert(parsed.host == "example.com");
    assert(parsed.port == 8080);
    assert(parsed.path == "/path");
    assert(parsed.query == "query=value");
    
    // Test default port
    auto parsed2 = conduit::parse_url("http://example.com/path");
    assert(parsed2.port == 80);
    
    std::cout << "✓ URL parsing tests passed" << std::endl;
}

void test_json_value_creation() {
    std::cout << "Testing JsonValue creation..." << std::endl;
    
    // Test different value types
    conduit::JsonValue null_val;
    assert(null_val.is_null());
    
    conduit::JsonValue bool_val(true);
    assert(bool_val.is_bool());
    assert(bool_val.as_bool() == true);
    
    conduit::JsonValue num_val(42.5);
    assert(num_val.is_number());
    assert(num_val.as_number() == 42.5);
    
    conduit::JsonValue str_val(std::string("hello"));
    assert(str_val.is_string());
    assert(str_val.as_string() == "hello");
    
    std::cout << "✓ JsonValue creation tests passed" << std::endl;
}

int main() {
    std::cout << "Running Conduit C++ Library Tests" << std::endl;
    std::cout << "==================================" << std::endl;
    
    try {
        test_json_value_creation();
        test_json_parsing();
        test_json_serialization();
        test_url_parsing();
        
        std::cout << std::endl;
        std::cout << "🎉 All tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

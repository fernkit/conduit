#include <iostream>
#include <conduit.hpp>

int main() {
    try {
        // JSON Parsing Example
        std::cout << "=== JSON Parsing Example ===" << std::endl;
        
        std::string json_str = R"({
            "name": "John Doe",
            "age": 30,
            "city": "New York",
            "active": true,
            "scores": [85, 90, 78],
            "address": {
                "street": "123 Main St",
                "zip": "10001"
            }
        })";
        
        auto json = conduit::parse_json(json_str);
        if (!json) {
            std::cerr << "Failed to parse JSON!" << std::endl;
            return 1;
        }
        
        std::cout << "JSON parsed successfully!" << std::endl;
        
        // Access object fields
        if (json->is_object()) {
            auto name = json->get_string("name");
            auto age = json->get_int("age");
            auto active = json->get_bool("active");
            
            std::cout << "Name: " << (name ? *name : "N/A") << std::endl;
            std::cout << "Age: " << (age ? std::to_string(*age) : "N/A") << std::endl;
            std::cout << "Active: " << (active ? (*active ? "true" : "false") : "N/A") << std::endl;
        }
        
        // JSON Serialization Example
        std::cout << "\n=== JSON Serialization Example ===" << std::endl;
        
        // Create JSON object using the new simplified API
        auto person = std::make_shared<std::map<std::string, std::shared_ptr<conduit::JsonValue>>>();
        (*person)["name"] = std::make_shared<conduit::JsonValue>("Jane Smith");
        (*person)["age"] = std::make_shared<conduit::JsonValue>(25.0);
        (*person)["student"] = std::make_shared<conduit::JsonValue>(true);
        
        // Create nested object
        auto contact = std::make_shared<std::map<std::string, std::shared_ptr<conduit::JsonValue>>>();
        (*contact)["email"] = std::make_shared<conduit::JsonValue>("jane@example.com");
        (*contact)["phone"] = std::make_shared<conduit::JsonValue>("555-1234");
        
        conduit::JsonValue contact_json;
        contact_json.set_object(contact);
        (*person)["contact"] = std::make_shared<conduit::JsonValue>(contact_json);
        
        // Create array
        auto hobbies = std::make_shared<std::vector<std::shared_ptr<conduit::JsonValue>>>();
        hobbies->push_back(std::make_shared<conduit::JsonValue>("reading"));
        hobbies->push_back(std::make_shared<conduit::JsonValue>("swimming"));
        hobbies->push_back(std::make_shared<conduit::JsonValue>("coding"));
        
        conduit::JsonValue hobbies_json;
        hobbies_json.set_array(hobbies);
        (*person)["hobbies"] = std::make_shared<conduit::JsonValue>(hobbies_json);
        
        conduit::JsonValue person_json;
        person_json.set_object(person);
        std::string serialized = conduit::serialize_json(person_json);
        
        std::cout << "Serialized JSON: " << serialized << std::endl;
        
        // HTTP Request with JSON
        std::cout << "\n=== HTTP Request with JSON ===" << std::endl;
        
        conduit::HttpClient client;
        
        // Use the serialized JSON for a POST request
        auto response = client.post_json("http://jsonplaceholder.typicode.com/posts", person_json);
        
        std::cout << "Response Status: " << response.status_code() << std::endl;
        std::cout << "Response Body: " << response.body() << std::endl;
        
        // Parse response JSON
        if (response.json()) {
            std::cout << "Response contains JSON data" << std::endl;
            
            if (response.json()->is_object()) {
                auto id = response.json()->get_int("id");
                if (id) {
                    std::cout << "Created resource with ID: " << *id << std::endl;
                }
            }
        }
        
        std::cout << "\nJSON examples completed successfully!" << std::endl;
        
    } catch (const conduit::HttpException& e) {
        std::cerr << "HTTP Error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

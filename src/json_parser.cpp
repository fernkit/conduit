#include "conduit.hpp"
#include <iostream>
#include <sstream>
#include <cctype>
#include <stdexcept>

namespace conduit {

namespace {
    /**
     * @brief JSON parser implementation
     */
    class JsonParser {
    public:
        explicit JsonParser(const std::string& json) : input_(json), pos_(0) {}
        
        std::optional<JsonValue> parse() {
            skip_whitespace();
            if (pos_ >= input_.length()) {
                return std::nullopt;
            }
            
            try {
                auto result = parse_value();
                skip_whitespace();
                if (pos_ < input_.length()) {
                    throw std::runtime_error("Unexpected characters after JSON value");
                }
                return result;
            } catch (const std::exception&) {
                return std::nullopt;
            }
        }
        
    private:
        std::string input_;
        size_t pos_;
        
        void skip_whitespace() {
            while (pos_ < input_.length() && std::isspace(input_[pos_])) {
                pos_++;
            }
        }
        
        char peek() const {
            return pos_ < input_.length() ? input_[pos_] : '\0';
        }
        
        char consume() {
            return pos_ < input_.length() ? input_[pos_++] : '\0';
        }
        
        void expect(char expected) {
            if (consume() != expected) {
                throw std::runtime_error("Expected '" + std::string(1, expected) + "'");
            }
        }
        
        JsonValue parse_value() {
            skip_whitespace();
            
            switch (peek()) {
                case 'n': return parse_null();
                case 't': case 'f': return parse_boolean();
                case '"': return parse_string();
                case '[': return parse_array();
                case '{': return parse_object();
                case '-': case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                    return parse_number();
                default:
                    throw std::runtime_error("Unexpected character in JSON");
            }
        }
        
        JsonValue parse_null() {
            if (input_.substr(pos_, 4) == "null") {
                pos_ += 4;
                return JsonValue{};
            }
            throw std::runtime_error("Expected 'null'");
        }
        
        JsonValue parse_boolean() {
            if (input_.substr(pos_, 4) == "true") {
                pos_ += 4;
                return JsonValue{true};
            } else if (input_.substr(pos_, 5) == "false") {
                pos_ += 5;
                return JsonValue{false};
            }
            throw std::runtime_error("Expected boolean value");
        }
        
        JsonValue parse_string() {
            expect('"');
            std::string result;
            
            while (peek() != '"' && peek() != '\0') {
                char c = consume();
                if (c == '\\') {
                    // Handle escape sequences
                    char escaped = consume();
                    switch (escaped) {
                        case '"': result += '"'; break;
                        case '\\': result += '\\'; break;
                        case '/': result += '/'; break;
                        case 'b': result += '\b'; break;
                        case 'f': result += '\f'; break;
                        case 'n': result += '\n'; break;
                        case 'r': result += '\r'; break;
                        case 't': result += '\t'; break;
                        default:
                            result += escaped;
                            break;
                    }
                } else {
                    result += c;
                }
            }
            
            expect('"');
            return JsonValue{result};
        }
        
        JsonValue parse_number() {
            size_t start = pos_;
            
            // Handle negative sign
            if (peek() == '-') {
                consume();
            }
            
            // Parse integer part
            if (peek() == '0') {
                consume();
            } else if (std::isdigit(peek())) {
                while (std::isdigit(peek())) {
                    consume();
                }
            } else {
                throw std::runtime_error("Invalid number format");
            }
            
            // Parse decimal part
            if (peek() == '.') {
                consume();
                if (!std::isdigit(peek())) {
                    throw std::runtime_error("Invalid number format");
                }
                while (std::isdigit(peek())) {
                    consume();
                }
            }
            
            // Parse exponent part
            if (peek() == 'e' || peek() == 'E') {
                consume();
                if (peek() == '+' || peek() == '-') {
                    consume();
                }
                if (!std::isdigit(peek())) {
                    throw std::runtime_error("Invalid number format");
                }
                while (std::isdigit(peek())) {
                    consume();
                }
            }
            
            std::string number_str = input_.substr(start, pos_ - start);
            return JsonValue{std::stod(number_str)};
        }
        
        JsonValue parse_array() {
            expect('[');
            skip_whitespace();
            
            auto array = std::make_shared<std::vector<std::shared_ptr<JsonValue>>>();
            
            if (peek() == ']') {
                consume();
                JsonValue result;
                result.set_array(array);
                return result;
            }
            
            while (true) {
                array->push_back(std::make_shared<JsonValue>(parse_value()));
                skip_whitespace();
                
                if (peek() == ']') {
                    consume();
                    break;
                } else if (peek() == ',') {
                    consume();
                    skip_whitespace();
                } else {
                    throw std::runtime_error("Expected ',' or ']' in array");
                }
            }
            
            JsonValue result;
            result.set_array(array);
            return result;
        }
        
        JsonValue parse_object() {
            expect('{');
            skip_whitespace();
            
            auto object = std::make_shared<std::map<std::string, std::shared_ptr<JsonValue>>>();
            
            if (peek() == '}') {
                consume();
                JsonValue result;
                result.set_object(object);
                return result;
            }
            
            while (true) {
                // Parse key
                if (peek() != '"') {
                    throw std::runtime_error("Expected string key in object");
                }
                JsonValue key_value = parse_string();
                std::string key = key_value.as_string();
                
                skip_whitespace();
                expect(':');
                skip_whitespace();
                
                // Parse value
                (*object)[key] = std::make_shared<JsonValue>(parse_value());
                skip_whitespace();
                
                if (peek() == '}') {
                    consume();
                    break;
                } else if (peek() == ',') {
                    consume();
                    skip_whitespace();
                } else {
                    throw std::runtime_error("Expected ',' or '}' in object");
                }
            }
            
            JsonValue result;
            result.set_object(object);
            return result;
        }
    };
    
    /**
     * @brief JSON serializer implementation
     */
    class JsonSerializer {
    public:
        static std::string serialize(const JsonValue& value) {
            JsonSerializer serializer;
            return serializer.serialize_value(value);
        }
        
    private:
        std::string serialize_value(const JsonValue& value) {
            switch (value.type()) {
                case JsonType::Null:
                    return "null";
                case JsonType::Boolean:
                    return value.as_bool() ? "true" : "false";
                case JsonType::Number:
                    return serialize_number(value.as_number());
                case JsonType::String:
                    return serialize_string(value.as_string());
                case JsonType::Array:
                    return serialize_array(value.as_array());
                case JsonType::Object:
                    return serialize_object(value.as_object());
                default:
                    throw std::runtime_error("Unknown JSON type");
            }
        }
        
        std::string serialize_number(double number) {
            // Check if it's an integer
            if (number == static_cast<long long>(number)) {
                return std::to_string(static_cast<long long>(number));
            } else {
                return std::to_string(number);
            }
        }
        
        std::string serialize_string(const std::string& str) {
            std::string result = "\"";
            for (char c : str) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\b': result += "\\b"; break;
                    case '\f': result += "\\f"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default:
                        result += c;
                        break;
                }
            }
            result += "\"";
            return result;
        }
        
        std::string serialize_array(const std::shared_ptr<std::vector<std::shared_ptr<JsonValue>>>& array) {
            std::string result = "[";
            if (array) {
                for (size_t i = 0; i < array->size(); ++i) {
                    if (i > 0) result += ",";
                    result += serialize_value(*(*array)[i]);
                }
            }
            result += "]";
            return result;
        }
        
        std::string serialize_object(const std::shared_ptr<std::map<std::string, std::shared_ptr<JsonValue>>>& object) {
            std::string result = "{";
            if (object) {
                bool first = true;
                for (const auto& [key, value] : *object) {
                    if (!first) result += ",";
                    first = false;
                    result += serialize_string(key) + ":" + serialize_value(*value);
                }
            }
            result += "}";
            return result;
        }
    };
} // anonymous namespace

std::optional<JsonValue> parse_json(const std::string& json_string) {
    JsonParser parser(json_string);
    return parser.parse();
}

std::string serialize_json(const JsonValue& value) {
    return JsonSerializer::serialize(value);
}

} // namespace conduit

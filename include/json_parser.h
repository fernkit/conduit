#ifndef JSON_PARSER_H
#define JSON_PARSER_H

typedef enum {
    JSON_NULL,
    JSON_BOOLEAN,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} JsonType;

typedef struct JsonObject JsonObject; 
typedef struct JsonValue JsonValue;
// forward decleration for, circular dependencies

struct JsonValue {
    JsonType type;
    union {
        int boolean;
        double number;
        char* string;
        struct {
            JsonValue** items;
            int count;
        } array;
        JsonObject* object;
    } value;
};



struct JsonObject {
    char** keys;
    JsonValue** values;
    int count;
};

int json_get_int(JsonObject* obj, const char* key);
const char* json_get_string(JsonObject* obj, const char* key);
int json_get_bool(JsonObject* obj, const char* key);
JsonValue* conduit_parse_json(const char* json_string);
void json_free_value(JsonValue* value);
void json_free_object(JsonObject* obj);

#endif // JSON_PARSER_H
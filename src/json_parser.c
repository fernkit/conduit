#include "../include/json_parser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// first principles json parser
// recursive descent parser


static const char* skip_ws(const char** input) {
    while (**input && isspace(**input)) (*input)++;
    return *input;
}

// "Rishi"
static char* parse_string(const char** input) {
    if(**input != '"') return NULL;
    (*input)++;

    const char* start = *input;
    while (**input && **input != '"') (*input)++;
    if(**input != '"') return NULL;

    size_t len = *input - start;
    char* result = malloc(len+1);
    memcpy(result, start, len);
    result[len] = '\0';
    (*input)++;
    return result;
}

static double parse_number (const char** input){
    char* end;
    double result = strtod(*input, &end);
    *input = end;
    return result;
}

static JsonValue* parse_value(const char** input);
static JsonObject* parse_object(const char** input);

static JsonObject* parse_object(const char** input) {
    if((**input != '{')) return NULL;
    (*input)++;

    JsonObject* obj = malloc(sizeof(JsonObject));
    if (!obj) return NULL; 
    
    obj->keys = NULL;
    obj->values = NULL;
    obj->count = 0;

    *input = skip_ws(input); 

    if (**input == '}') {
        (*input)++;
        return obj;
    }

    while (1) {        
        *input = skip_ws(input);
        if (**input != '"') {
            json_free_object(obj); 
            return NULL;
        }

        char* key = parse_string(input);
        if (!key) {
            json_free_object(obj); 
            return NULL;
        }

        obj->keys = realloc(obj->keys, (obj->count + 1) * sizeof(char*));
        if (!obj->keys) {
            free(key);             
            json_free_object(obj); 
            return NULL;
        }
        obj->keys[obj->count] = key;

        *input = skip_ws(input);
        if (**input == ':') {
            (*input)++;
        } else {
            json_free_object(obj);
            return NULL;
        }
        
        *input = skip_ws(input);
        JsonValue* value = parse_value(input);
        if (!value) {
            json_free_object(obj);
            return NULL;
        }

        obj->values = realloc(obj->values, (obj->count + 1) * sizeof(JsonValue*));
        if (!obj->values) {
            json_free_value(value);
            json_free_object(obj);  
            return NULL;
        }
        obj->values[obj->count] = value;
        obj->count++;

        *input = skip_ws(input);
        if (**input == ',') {
            (*input)++; 
            continue;
        } else if (**input == '}') {
            (*input)++; 
            return obj;
        } else {
            json_free_object(obj);  // Free memory if error
            return NULL;
        }
    }
}



JsonValue* parse_value(const char** input) {
    *input = skip_ws(input);
    
    JsonValue* value = malloc(sizeof(JsonValue));
    
    if (**input == '{') {
        value->type = JSON_OBJECT;
        value->value.object = parse_object(input);
    } 
    else if (**input == '"') {
        value->type = JSON_STRING;
        value->value.string = parse_string(input);
    }
    else if (isdigit(**input) || **input == '-') {
        value->type = JSON_NUMBER;
        value->value.number = parse_number(input);
    }
    else if (strncmp(*input, "true", 4) == 0) {
        value->type = JSON_BOOLEAN;
        value->value.boolean = 1;
        *input += 4;
    } else if (strncmp(*input, "false", 5) == 0) {
        value->type = JSON_BOOLEAN;
        value->value.boolean = 0;
        *input += 5;
    } else if (strncmp(*input, "null", 4) == 0) {
        value->type = JSON_NULL;
        *input += 4;
    }
    else {
        // invalid JSON
        free(value);
        return NULL;
    }    
    return value;
}


JsonValue* conduit_parse_json(const char* json_string) {
    return parse_value(&json_string);
}


JsonValue* json_get_value(JsonObject* obj, const char* key) {
    if (!obj) return NULL;
    
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->keys[i], key) == 0) {
            return obj->values[i];
        }
    }
    
    return NULL;
}

int json_get_int(JsonObject* obj, const char* key) {
    JsonValue* value = json_get_value(obj, key);
    
    if (!value || value->type != JSON_NUMBER) {
        return 0;
    }
    
    return (int)value->value.number;
}


const char* json_get_string(JsonObject* obj, const char* key) {
    JsonValue* value = json_get_value(obj, key);
    
    if (!value || value->type != JSON_STRING) {
        return NULL;
    }
    
    return value->value.string;
}

int json_get_bool(JsonObject* obj, const char* key) {
    JsonValue* value = json_get_value(obj, key);
    
    if (!value || value->type != JSON_BOOLEAN) {
        return 0;
    }
    
    return value->value.boolean;
}

void json_free_object(JsonObject* obj) {
    if (!obj) return;
    
    for (int i = 0; i < obj->count; i++) {
        if (obj->keys && obj->keys[i]) free(obj->keys[i]);
        if (obj->values && obj->values[i]) json_free_value(obj->values[i]);
    }
    
    free(obj->keys);
    free(obj->values);
    free(obj);
}

void json_free_value(JsonValue* value) {
    if (!value) return;
    
    switch (value->type) {
        case JSON_STRING:
            free(value->value.string);
            break;
        case JSON_OBJECT:
            json_free_object(value->value.object);
            break;
        case JSON_ARRAY:
            // TODO: make array implementation 🤓
            break;
        default:
            break;
    }
    
    free(value);
}
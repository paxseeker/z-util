#include <stdio.h>
#include <stdlib.h>

#define Z_JSON_IMPLEMENTATION
#include "z-json.h"

int main(void) {
    const char *json_str = 
        "{\n"
        "  \"name\": \"z-json\",\n"
        "  \"version\": 1.0,\n"
        "  \"active\": true,\n"
        "  \"tags\": [\"json\", \"parser\", \"stb-style\"],\n"
        "  \"author\": {\n"
        "    \"name\": \"zjx\",\n"
        "    \"unicode\": \"\\u4F60\\u597D\"\n"
        "  },\n"
        "  \"null_val\": null\n"
        "}";

    printf("Parsing JSON string:\n%s\n\n", json_str);

    ZJson *root = z_json_parse_str(json_str);
    if (!root) {
        printf("Failed to parse JSON\n");
        return 1;
    }

    ZJson *name = z_json_object_get(root, "name");
    if (name && name->type == Z_JSON_STRING) {
        printf("name: %s\n", name->value.str_val);
    }

    ZJson *version = z_json_object_get(root, "version");
    if (version && version->type == Z_JSON_NUMBER) {
        printf("version: %g\n", version->value.num_val);
    }

    ZJson *active = z_json_object_get(root, "active");
    if (active && active->type == Z_JSON_BOOL) {
        printf("active: %s\n", active->value.bool_val ? "true" : "false");
    }

    ZJson *tags = z_json_object_get(root, "tags");
    if (tags && tags->type == Z_JSON_ARRAY) {
        printf("tags: [");
        for (size_t i = 0; i < z_json_array_size(tags); i++) {
            ZJson *tag = z_json_array_get(tags, i);
            if (tag && tag->type == Z_JSON_STRING) {
                printf("%s\"%s\"", i > 0 ? ", " : "", tag->value.str_val);
            }
        }
        printf("]\n");
    }

    ZJson *author = z_json_object_get(root, "author");
    if (author && author->type == Z_JSON_OBJECT) {
        ZJson *author_name = z_json_object_get(author, "name");
        ZJson *unicode_val = z_json_object_get(author, "unicode");
        if (author_name && author_name->type == Z_JSON_STRING) {
            printf("author.name: %s\n", author_name->value.str_val);
        }
        if (unicode_val && unicode_val->type == Z_JSON_STRING) {
            printf("author.unicode: %s\n", unicode_val->value.str_val);
        }
    }

    ZJson *path_test = z_json_get_path(root, "author.name");
    if (path_test && path_test->type == Z_JSON_STRING) {
        printf("path author.name: %s\n", path_test->value.str_val);
    }

    ZJson *tag0 = z_json_get_path(root, "tags[0]");
    if (tag0 && tag0->type == Z_JSON_STRING) {
        printf("path tags[0]: %s\n", tag0->value.str_val);
    }

    ZJson *null_val = z_json_object_get(root, "null_val");
    if (null_val && null_val->type == Z_JSON_NULL) {
        printf("null_val: null\n");
    }

    printf("\n--- Testing file read ---\n");
    ZJson *file_root = z_json_parse_file("test_data.json");
    if (!file_root) {
        printf("Failed to parse JSON from file\n");
    } else {
        ZJson *file_name = z_json_object_get(file_root, "name");
        if (file_name && file_name->type == Z_JSON_STRING) {
            printf("file name: %s\n", file_name->value.str_val);
        }
        z_json_free(file_root);
    }

    size_t json_len;
    char *serialized = z_json_stringify(root, &json_len);
    if (serialized) {
        printf("\nStringified JSON:\n%s\n", serialized);
        free(serialized);
    }

    z_json_free(root);

    printf("\n--- Testing JSON building and modification ---\n");

    ZJson *obj = z_json_create_object();
    z_json_object_set(obj, "name", z_json_create_string("test"));
    z_json_object_set(obj, "version", z_json_create_number(2.5));
    z_json_object_set(obj, "active", z_json_create_bool(true));
    z_json_object_set(obj, "null_val", z_json_create_null());

    ZJson *arr = z_json_create_array();
    z_json_array_append(arr, z_json_create_string("a"));
    z_json_array_append(arr, z_json_create_number(1));
    z_json_array_append(arr, z_json_create_bool(false));
    z_json_object_set(obj, "items", arr);

    serialized = z_json_stringify(obj, &json_len);
    if (serialized) {
        printf("Built object: %s\n", serialized);
        free(serialized);
    }

    z_json_object_remove(obj, "null_val");
    z_json_array_remove(arr, 1);
    z_json_object_set(obj, "added", z_json_create_string("new"));

    serialized = z_json_stringify(obj, &json_len);
    if (serialized) {
        printf("Modified object: %s\n", serialized);
        free(serialized);
    }

    if (z_json_write_file(obj, "output.json")) {
        printf("Written to output.json\n");
    }

    printf("\n--- Testing pretty output ---\n");
    ZJson *pretty_obj = z_json_create_object();
    z_json_object_set(pretty_obj, "name", z_json_create_string("test"));
    z_json_object_set(pretty_obj, "version", z_json_create_number(1.0));

    ZJson *nested = z_json_create_object();
    z_json_object_set(nested, "key", z_json_create_string("value"));
    z_json_object_set(pretty_obj, "nested", nested);

    ZJson *pretty_arr = z_json_create_array();
    z_json_array_append(pretty_arr, z_json_create_number(1));
    z_json_array_append(pretty_arr, z_json_create_number(2));
    z_json_object_set(pretty_obj, "array", pretty_arr);

    size_t pretty_len;
    char *    pretty_str = z_json_stringify_pretty(pretty_obj, &pretty_len, 2);
    if (pretty_str) {
        printf("Pretty JSON (indent=2):\n%s\n", pretty_str);
        free(pretty_str);
    }

    if (z_json_write_file_pretty(pretty_obj, "pretty_output.json", Z_JSON_DEFAULT_INDENT)) {
        printf("Written pretty JSON to pretty_output.json with indent=%d\n", Z_JSON_DEFAULT_INDENT);
    }

    z_json_free(pretty_obj);
    z_json_free(obj);
    return 0;
}

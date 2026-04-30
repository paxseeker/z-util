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

    size_t json_len;
    char *serialized = z_json_stringify(root, &json_len);
    if (serialized) {
        printf("\nStringified JSON:\n%s\n", serialized);
        free(serialized);
    }

    z_json_free(root);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>

#define Z_YAML_IMPLEMENTATION
#include "z-yaml.h"

int main(void) {
    const char *yaml_str =
        "name: z-yaml\n"
        "version: 1.0\n"
        "active: true\n"
        "tags:\n"
        "  - yaml\n"
        "  - parser\n"
        "  - stb-style\n"
        "author:\n"
        "  name: zjx\n"
        "  unicode: \"\\u4F60\\u597D\"\n"
        "null_val: null\n";

    printf("Parsing YAML string:\n%s\n\n", yaml_str);

    ZYaml *root = z_yaml_parse_str(yaml_str);
    if (!root) {
        printf("Failed to parse YAML\n");
        return 1;
    }

    ZYaml *name = z_yaml_object_get(root, "name");
    if (name && name->type == Z_YAML_STRING) {
        printf("name: %s\n", name->value.str_val);
    }

    ZYaml *version = z_yaml_object_get(root, "version");
    if (version && version->type == Z_YAML_NUMBER) {
        printf("version: %g\n", version->value.num_val);
    }

    ZYaml *active = z_yaml_object_get(root, "active");
    if (active && active->type == Z_YAML_BOOL) {
        printf("active: %s\n", active->value.bool_val ? "true" : "false");
    }

    ZYaml *tags = z_yaml_object_get(root, "tags");
    if (tags && tags->type == Z_YAML_ARRAY) {
        printf("tags: [");
        for (size_t i = 0; i < z_yaml_array_size(tags); i++) {
            ZYaml *tag = z_yaml_array_get(tags, i);
            if (tag && tag->type == Z_YAML_STRING) {
                printf("%s\"%s\"", i > 0 ? ", " : "", tag->value.str_val);
            }
        }
        printf("]\n");
    }

    ZYaml *author = z_yaml_object_get(root, "author");
    if (author && author->type == Z_YAML_OBJECT) {
        ZYaml *author_name = z_yaml_object_get(author, "name");
        if (author_name && author_name->type == Z_YAML_STRING) {
            printf("author.name: %s\n", author_name->value.str_val);
        }
    }

    ZYaml *path_test = z_yaml_get_path(root, "author.name");
    if (path_test && path_test->type == Z_YAML_STRING) {
        printf("path author.name: %s\n", path_test->value.str_val);
    }

    ZYaml *tag0 = z_yaml_get_path(root, "tags[0]");
    if (tag0 && tag0->type == Z_YAML_STRING) {
        printf("path tags[0]: %s\n", tag0->value.str_val);
    }

    ZYaml *null_val = z_yaml_object_get(root, "null_val");
    if (null_val && null_val->type == Z_YAML_NULL) {
        printf("null_val: null\n");
    }

    printf("\n--- Testing YAML building and modification ---\n");

    ZYaml *obj = z_yaml_create_object();
    z_yaml_object_set(obj, "name", z_yaml_create_string("test"));
    z_yaml_object_set(obj, "version", z_yaml_create_number(2.5));
    z_yaml_object_set(obj, "active", z_yaml_create_bool(true));
    z_yaml_object_set(obj, "null_val", z_yaml_create_null());

    ZYaml *arr = z_yaml_create_array();
    z_yaml_array_append(arr, z_yaml_create_string("a"));
    z_yaml_array_append(arr, z_yaml_create_number(1));
    z_yaml_array_append(arr, z_yaml_create_bool(false));
    z_yaml_object_set(obj, "items", arr);

    size_t yaml_len;
    char *serialized = z_yaml_stringify(obj, &yaml_len);
    if (serialized) {
        printf("Built object: %s\n", serialized);
        free(serialized);
    }

    z_yaml_object_remove(obj, "null_val");
    z_yaml_array_remove(arr, 1);
    z_yaml_object_set(obj, "added", z_yaml_create_string("new"));

    serialized = z_yaml_stringify(obj, &yaml_len);
    if (serialized) {
        printf("Modified object: %s\n", serialized);
        free(serialized);
    }

    if (z_yaml_write_file(obj, "output.yaml")) {
        printf("Written to output.yaml\n");
    }

    printf("\n--- Testing pretty output ---\n");
    ZYaml *pretty_obj = z_yaml_create_object();
    z_yaml_object_set(pretty_obj, "name", z_yaml_create_string("test"));
    z_yaml_object_set(pretty_obj, "version", z_yaml_create_number(1.0));

    ZYaml *nested = z_yaml_create_object();
    z_yaml_object_set(nested, "key", z_yaml_create_string("value"));
    z_yaml_object_set(pretty_obj, "nested", nested);

    ZYaml *pretty_arr = z_yaml_create_array();
    z_yaml_array_append(pretty_arr, z_yaml_create_number(1));
    z_yaml_array_append(pretty_arr, z_yaml_create_number(2));
    z_yaml_object_set(pretty_obj, "array", pretty_arr);

    size_t pretty_len;
    char *pretty_str = z_yaml_stringify_pretty(pretty_obj, &pretty_len, 2);
    if (pretty_str) {
        printf("Pretty YAML (indent=2):\n%s\n", pretty_str);
        free(pretty_str);
    }

    if (z_yaml_write_file_pretty(pretty_obj, "pretty_output.yaml", Z_YAML_DEFAULT_INDENT)) {
        printf("Written pretty YAML to pretty_output.yaml with indent=%d\n", Z_YAML_DEFAULT_INDENT);
    }

    z_yaml_free(pretty_obj);
    z_yaml_free(obj);
    z_yaml_free(root);
    return 0;
}
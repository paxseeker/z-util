#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Z_JSON_IMPLEMENTATION
#include "z-json.h"

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("Test %d: %s\n", test_count, name); } while(0)
#define PASS() do { pass_count++; printf("  PASS\n"); } while(0)
#define FAIL(msg) do { printf("  FAIL: %s\n", msg); } while(0)

int main(void) {
    printf("=== Complex JSON Parser Tests ===\n\n");

    const char *complex_json =
        "{\n"
        "  \"user\": {\n"
        "    \"id\": 12345,\n"
        "    \"name\": \"张三\",\n"
        "    \"email\": \"zhangsan@example.com\",\n"
        "    \"active\": true,\n"
        "    \"score\": 98.6,\n"
        "    \"tags\": [\"vip\", \"verified\", \"premium\"],\n"
        "    \"metadata\": {\n"
        "      \"created\": \"2024-01-15\",\n"
        "      \"login_history\": [\n"
        "        {\"date\": \"2024-01-10\", \"ip\": \"192.168.1.1\"},\n"
        "        {\"date\": \"2024-01-12\", \"ip\": \"10.0.0.1\"},\n"
        "        {\"date\": \"2024-01-15\", \"ip\": \"172.16.0.1\"}\n"
        "      ]\n"
        "    },\n"
        "    \"settings\": {\n"
        "      \"theme\": {\n"
        "        \"dark_mode\": true,\n"
        "        \"color\": \"blue\",\n"
        "        \"font_size\": 14\n"
        "      }\n"
        "    },\n"
        "    \"empty_array\": [],\n"
        "    \"empty_object\": {},\n"
        "    \"numbers\": {\n"
        "      \"negative\": -42,\n"
        "      \"zero\": 0,\n"
        "      \"decimal\": 3.14159,\n"
        "      \"scientific\": 1.5e10,\n"
        "      \"negative_exp\": -2.5e-3\n"
        "    }\n"
        "  },\n"
        "  \"null_value\": null,\n"
        "  \"escaped_string\": \"Hello\\nWorld\\tTest\\\"Quote\\\"\",\n"
        "  \"unicode_text\": \"\\u4F60\\u597D\\u4E16\\u754C\"\n"
        "}";

    ZJson *root = z_json_parse_str(complex_json);
    if (!root) {
        printf("FATAL: Failed to parse complex JSON\n");
        return 1;
    }

    TEST("Parse complex JSON");
    PASS();

    TEST("Get user.name via path");
    ZJson *val = z_json_get_path(root, "user.name");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "张三") == 0) {
        PASS();
    } else {
        FAIL("Expected '张三'");
    }

    TEST("Get user.id via path");
    val = z_json_get_path(root, "user.id");
    if (val && val->type == Z_JSON_NUMBER && val->value.num_val == 12345) {
        PASS();
    } else {
        FAIL("Expected 12345");
    }

    TEST("Get user.score via path");
    val = z_json_get_path(root, "user.score");
    if (val && val->type == Z_JSON_NUMBER && val->value.num_val == 98.6) {
        PASS();
    } else {
        FAIL("Expected 98.6");
    }

    TEST("Get user.active via path");
    val = z_json_get_path(root, "user.active");
    if (val && val->type == Z_JSON_BOOL && val->value.bool_val == true) {
        PASS();
    } else {
        FAIL("Expected true");
    }

    TEST("Get user.tags[0] via path");
    val = z_json_get_path(root, "user.tags[0]");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "vip") == 0) {
        PASS();
    } else {
        FAIL("Expected 'vip'");
    }

    TEST("Get user.tags[2] via path");
    val = z_json_get_path(root, "user.tags[2]");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "premium") == 0) {
        PASS();
    } else {
        FAIL("Expected 'premium'");
    }

    TEST("Get user.tags array size");
    ZJson *tags = z_json_get_path(root, "user.tags");
    if (tags && z_json_array_size(tags) == 3) {
        PASS();
    } else {
        FAIL("Expected array size 3");
    }

    TEST("Get nested metadata.created via path");
    val = z_json_get_path(root, "user.metadata.created");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "2024-01-15") == 0) {
        PASS();
    } else {
        FAIL("Expected '2024-01-15'");
    }

    TEST("Get nested login_history[1].ip via path");
    val = z_json_get_path(root, "user.metadata.login_history[1].ip");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "10.0.0.1") == 0) {
        PASS();
    } else {
        FAIL("Expected '10.0.0.1'");
    }

    TEST("Get triple nested settings.theme.dark_mode via path");
    val = z_json_get_path(root, "user.settings.theme.dark_mode");
    if (val && val->type == Z_JSON_BOOL && val->value.bool_val == true) {
        PASS();
    } else {
        FAIL("Expected true");
    }

    TEST("Get settings.theme.font_size via path");
    val = z_json_get_path(root, "user.settings.theme.font_size");
    if (val && val->type == Z_JSON_NUMBER && val->value.num_val == 14) {
        PASS();
    } else {
        FAIL("Expected 14");
    }

    TEST("Get empty_array size");
    ZJson *empty_arr = z_json_get_path(root, "user.empty_array");
    if (empty_arr && z_json_array_size(empty_arr) == 0) {
        PASS();
    } else {
        FAIL("Expected empty array");
    }

    TEST("Get empty_object");
    ZJson *empty_obj = z_json_get_path(root, "user.empty_object");
    if (empty_obj && empty_obj->type == Z_JSON_OBJECT) {
        PASS();
    } else {
        FAIL("Expected empty object");
    }

    TEST("Get negative number via path");
    val = z_json_get_path(root, "user.numbers.negative");
    if (val && val->type == Z_JSON_NUMBER && val->value.num_val == -42) {
        PASS();
    } else {
        FAIL("Expected -42");
    }

    TEST("Get decimal number via path");
    val = z_json_get_path(root, "user.numbers.decimal");
    if (val && val->type == Z_JSON_NUMBER && val->value.num_val == 3.14159) {
        PASS();
    } else {
        FAIL("Expected 3.14159");
    }

    TEST("Get scientific notation number via path");
    val = z_json_get_path(root, "user.numbers.scientific");
    if (val && val->type == Z_JSON_NUMBER && val->value.num_val == 1.5e10) {
        PASS();
    } else {
        FAIL("Expected 1.5e10");
    }

    TEST("Get null_value via path");
    val = z_json_get_path(root, "null_value");
    if (val && val->type == Z_JSON_NULL) {
        PASS();
    } else {
        FAIL("Expected null");
    }

    TEST("Get escaped string via path");
    val = z_json_get_path(root, "escaped_string");
    if (val && val->type == Z_JSON_STRING) {
        PASS();
    } else {
        FAIL("Expected escaped string");
    }

    TEST("Get unicode text via path");
    val = z_json_get_path(root, "unicode_text");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "你好世界") == 0) {
        PASS();
    } else {
        FAIL("Expected '你好世界'");
    }

    TEST("Invalid path returns NULL");
    val = z_json_get_path(root, "user.nonexistent");
    if (val == NULL) {
        PASS();
    } else {
        FAIL("Expected NULL");
    }

    TEST("Invalid array index returns NULL");
    val = z_json_get_path(root, "user.tags[99]");
    if (val == NULL) {
        PASS();
    } else {
        FAIL("Expected NULL for out of bounds");
    }

    TEST("Invalid nested path returns NULL");
    val = z_json_get_path(root, "user.metadata.nonexistent[0].key");
    if (val == NULL) {
        PASS();
    } else {
        FAIL("Expected NULL");
    }

    TEST("Stringify and re-parse");
    size_t json_len;
    char *str = z_json_stringify(root, &json_len);
    if (str) {
        ZJson *reparsed = z_json_parse_str(str);
        if (reparsed) {
            val = z_json_get_path(reparsed, "user.name");
            if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "张三") == 0) {
                PASS();
            } else {
                FAIL("Re-parsed JSON missing data");
            }
            z_json_free(reparsed);
        } else {
            FAIL("Failed to re-parse stringified JSON");
        }
        free(str);
    } else {
        FAIL("Stringify returned NULL");
    }

    TEST("Parse from file");
    FILE *f = fopen("test_complex.json", "w");
    if (f) {
        fprintf(f, "%s", complex_json);
        fclose(f);
        ZJson *from_file = z_json_parse_file("test_complex.json");
        if (from_file) {
            val = z_json_get_path(from_file, "user.settings.theme.color");
            if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "blue") == 0) {
                PASS();
            } else {
                FAIL("Failed to get value from file-parsed JSON");
            }
            z_json_free(from_file);
        } else {
            FAIL("Failed to parse from file");
        }
        remove("test_complex.json");
    } else {
        FAIL("Could not create test file");
    }

    z_json_free(root);

    printf("\n=== Test Results ===\n");
    printf("Total: %d, Passed: %d, Failed: %d\n", test_count, pass_count, test_count - pass_count);

    return (pass_count == test_count) ? 0 : 1;
}

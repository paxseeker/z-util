#ifndef Z_YAML_H
#define Z_YAML_H

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define Z_YAML_DEFAULT_INDENT 2

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    Z_YAML_NULL,
    Z_YAML_STRING,
    Z_YAML_NUMBER,
    Z_YAML_BOOL,
    Z_YAML_OBJECT,
    Z_YAML_ARRAY
} ZYamlType;

typedef struct ZYaml {
    ZYamlType type;
    union {
        const char *str_val;
        double num_val;
        bool bool_val;
        struct {
            struct ZYaml **items;
            size_t count;
            size_t capacity;
        } array;
        struct {
            const char **keys;
            struct ZYaml **values;
            size_t count;
            size_t capacity;
        } object;
    } value;
} ZYaml;

ZYaml *z_yaml_parse_str(const char *str);
ZYaml *z_yaml_parse_file(const char *filename);
bool z_yaml_write_file(ZYaml *yaml, const char *filename);
bool z_yaml_write_file_pretty(ZYaml *yaml, const char *filename, int indent);

ZYaml *z_yaml_create_null(void);
ZYaml *z_yaml_create_string(const char *str);
ZYaml *z_yaml_create_number(double num);
ZYaml *z_yaml_create_bool(bool val);
ZYaml *z_yaml_create_object(void);
ZYaml *z_yaml_create_array(void);

bool z_yaml_object_set(ZYaml *obj, const char *key, ZYaml *value);
bool z_yaml_object_remove(ZYaml *obj, const char *key);
bool z_yaml_array_append(ZYaml *arr, ZYaml *value);
bool z_yaml_array_remove(ZYaml *arr, size_t index);

ZYaml *z_yaml_object_get(ZYaml *obj, const char *key);
ZYaml *z_yaml_array_get(ZYaml *arr, size_t index);
size_t z_yaml_array_size(ZYaml *arr);
ZYaml *z_yaml_get_path(ZYaml *yaml, const char *path);
char *z_yaml_stringify(ZYaml *yaml, size_t *out_len);
char *z_yaml_stringify_pretty(ZYaml *yaml, size_t *out_len, int indent);
void z_yaml_free(ZYaml *yaml);

#ifdef __cplusplus
}
#endif

#endif /* Z_YAML_H */
#ifdef Z_YAML_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

typedef struct {
    const char *str;
    size_t pos;
    size_t len;
    int indent;
} z_yaml_parser;

typedef struct {
    const char *key;
    ZYaml *value;
    int indent;
} z_yaml_kv;

static void z_yaml_skip_whitespace(z_yaml_parser *p) {
    while (p->pos < p->len && (p->str[p->pos] == ' ' || p->str[p->pos] == '\t')) {
        p->pos++;
    }
}

static void z_yaml_skip_line(z_yaml_parser *p) {
    while (p->pos < p->len && p->str[p->pos] != '\n') {
        p->pos++;
    }
    if (p->pos < p->len) p->pos++;
}

static int z_yaml_get_indent(z_yaml_parser *p) {
    size_t start = p->pos;
    int indent = 0;
    while (p->pos < p->len && (p->str[p->pos] == ' ' || p->str[p->pos] == '\t')) {
        if (p->str[p->pos] == ' ') indent++;
        else indent += 8;
        p->pos++;
    }
    if (p->pos >= p->len || p->str[p->pos] == '\n' || p->str[p->pos] == '#') {
        p->pos = start;
        return -1;
    }
    return indent;
}

static bool z_yaml_is_newline(char c) {
    return c == '\n' || c == '\r';
}

static bool z_yaml_is_flow_char(char c) {
    return c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == ':';
}

static ZYaml *z_yaml_parse_value(z_yaml_parser *p);

static char *z_yaml_parse_string(z_yaml_parser *p, bool *flow_out) {
    if (p->pos >= p->len) return NULL;

    bool flow = false;
    if (p->str[p->pos] == '"' || p->str[p->pos] == '\'') {
        flow = true;
        char quote = p->str[p->pos];
        p->pos++;

        size_t start = p->pos;
        size_t len = 0;
        while (p->pos < p->len && p->str[p->pos] != quote) {
            if (p->str[p->pos] == '\\') {
                p->pos++;
                if (p->pos >= p->len) return NULL;
            }
            p->pos++;
            len++;
        }
        if (p->pos >= p->len) return NULL;
        p->pos++;

        char *result = (char *)malloc(len + 1);
        if (!result) return NULL;

        size_t rpos = 0;
        size_t pos = start;
        while (pos < p->pos - 1) {
            if (p->str[pos] == '\\') {
                pos++;
                switch (p->str[pos]) {
                    case '"': case '\\': case '/':
                        result[rpos++] = p->str[pos];
                        break;
                    case 'n': result[rpos++] = '\n'; break;
                    case 'r': result[rpos++] = '\r'; break;
                    case 't': result[rpos++] = '\t'; break;
                    case 'x': {
                        if (pos + 2 >= p->pos - 1) { free(result); return NULL; }
                        int codepoint = 0;
                        for (int i = 1; i <= 2; i++) {
                            char c = pos + i < p->pos - 1 ? p->str[pos + i] : '0';
                            codepoint <<= 4;
                            if (c >= '0' && c <= '9') codepoint |= c - '0';
                            else if (c >= 'a' && c <= 'f') codepoint |= c - 'a' + 10;
                            else if (c >= 'A' && c <= 'F') codepoint |= c - 'A' + 10;
                        }
                        result[rpos++] = (char)codepoint;
                        pos += 2;
                        break;
                    }
                    default: result[rpos++] = p->str[pos]; break;
                }
            } else {
                result[rpos++] = p->str[pos];
            }
            pos++;
        }
        result[rpos] = '\0';
        if (flow_out) *flow_out = flow;
        return result;
    }

    size_t start = p->pos;
    size_t len = 0;
    while (p->pos < p->len && !z_yaml_is_newline(p->str[p->pos]) && p->str[p->pos] != '#') {
        if (p->str[p->pos] == '\\') {
            p->pos++;
            if (p->pos >= p->len) break;
        }
        p->pos++;
        len++;
    }

    while (len > 0 && (p->str[start + len - 1] == ' ' || p->str[start + len - 1] == '\t')) {
        len--;
    }

    char *result = (char *)malloc(len + 1);
    if (!result) return NULL;
    memcpy(result, p->str + start, len);
    result[len] = '\0';
    if (flow_out) *flow_out = flow;
    return result;
}

static ZYaml *z_yaml_parse_number(z_yaml_parser *p) {
    size_t start = p->pos;
    bool negative = false;

    if (p->str[p->pos] == '-') {
        negative = true;
        p->pos++;
    }

    bool has_decimal = false;
    bool has_exponent = false;

    while (p->pos < p->len && (p->str[p->pos] >= '0' && p->str[p->pos] <= '9')) {
        p->pos++;
    }

    if (p->pos < p->len && p->str[p->pos] == '.') {
        has_decimal = true;
        p->pos++;
        while (p->pos < p->len && (p->str[p->pos] >= '0' && p->str[p->pos] <= '9')) {
            p->pos++;
        }
    }

    if (p->pos < p->len && (p->str[p->pos] == 'e' || p->str[p->pos] == 'E')) {
        has_exponent = true;
        p->pos++;
        if (p->pos < p->len && (p->str[p->pos] == '+' || p->str[p->pos] == '-')) {
            p->pos++;
        }
        while (p->pos < p->len && (p->str[p->pos] >= '0' && p->str[p->pos] <= '9')) {
            p->pos++;
        }
    }

    size_t len = p->pos - start;
    char *num_str = (char *)malloc(len + 1);
    if (!num_str) return NULL;
    memcpy(num_str, p->str + start, len);
    num_str[len] = '\0';

    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) { free(num_str); return NULL; }

    yaml->type = Z_YAML_NUMBER;
    yaml->value.num_val = strtod(num_str, NULL);
    free(num_str);
    return yaml;
}

static ZYaml *z_yaml_parse_object(z_yaml_parser *p, int base_indent);

static ZYaml *z_yaml_parse_array(z_yaml_parser *p, int base_indent) {
    if (p->pos >= p->len) return NULL;

    if (p->str[p->pos] == '[') {
        p->pos++;
        z_yaml_skip_whitespace(p);

        ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
        if (!yaml) return NULL;
        yaml->type = Z_YAML_ARRAY;
        yaml->value.array.items = NULL;
        yaml->value.array.count = 0;
        yaml->value.array.capacity = 0;

        while (p->pos < p->len) {
            z_yaml_skip_whitespace(p);
            if (p->pos < p->len && p->str[p->pos] == ']') {
                p->pos++;
                return yaml;
            }

            ZYaml *item = z_yaml_parse_value(p);
            if (!item) { z_yaml_free(yaml); return NULL; }

            if (yaml->value.array.count >= yaml->value.array.capacity) {
                size_t new_cap = yaml->value.array.capacity == 0 ? 8 : yaml->value.array.capacity * 2;
                ZYaml **new_items = (ZYaml **)realloc(yaml->value.array.items, new_cap * sizeof(ZYaml *));
                if (!new_items) { free(item); z_yaml_free(yaml); return NULL; }
                yaml->value.array.items = new_items;
                yaml->value.array.capacity = new_cap;
            }
            yaml->value.array.items[yaml->value.array.count++] = item;

            z_yaml_skip_whitespace(p);
            if (p->pos < p->len && p->str[p->pos] == ',') {
                p->pos++;
                z_yaml_skip_whitespace(p);
            }
        }

        return yaml;
    }

    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_ARRAY;
    yaml->value.array.items = NULL;
    yaml->value.array.count = 0;
    yaml->value.array.capacity = 0;

    while (p->pos < p->len) {
        z_yaml_skip_whitespace(p);

        if (p->pos >= p->len || z_yaml_is_newline(p->str[p->pos])) {
            if (p->pos < p->len && p->str[p->pos] == '\n') p->pos++;
            if (p->pos >= p->len) break;

            int indent = z_yaml_get_indent(p);
            if (indent < 0) continue;
            if (indent <= base_indent) {
                break;
            }
            continue;
        }

        if (p->str[p->pos] == '-') {
            p->pos++;
            z_yaml_skip_whitespace(p);

            ZYaml *item = z_yaml_parse_value(p);
            if (!item) { z_yaml_free(yaml); return NULL; }

            if (yaml->value.array.count >= yaml->value.array.capacity) {
                size_t new_cap = yaml->value.array.capacity == 0 ? 8 : yaml->value.array.capacity * 2;
                ZYaml **new_items = (ZYaml **)realloc(yaml->value.array.items, new_cap * sizeof(ZYaml *));
                if (!new_items) { free(item); z_yaml_free(yaml); return NULL; }
                yaml->value.array.items = new_items;
                yaml->value.array.capacity = new_cap;
            }
            yaml->value.array.items[yaml->value.array.count++] = item;
        } else {
            break;
        }
    }

    return yaml;
}

static ZYaml *z_yaml_parse_object(z_yaml_parser *p, int base_indent) {
    if (p->pos >= p->len) return NULL;

    if (p->str[p->pos] == '{') {
        p->pos++;
        z_yaml_skip_whitespace(p);

        ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
        if (!yaml) return NULL;
        yaml->type = Z_YAML_OBJECT;
        yaml->value.object.keys = NULL;
        yaml->value.object.values = NULL;
        yaml->value.object.count = 0;
        yaml->value.object.capacity = 0;

        while (p->pos < p->len) {
            z_yaml_skip_whitespace(p);
            if (p->pos < p->len && p->str[p->pos] == '}') {
                p->pos++;
                return yaml;
            }

            bool flow;
            char *key = z_yaml_parse_string(p, &flow);
            if (!key) { z_yaml_free(yaml); return NULL; }

            z_yaml_skip_whitespace(p);
            if (p->pos < p->len && p->str[p->pos] == ':') {
                p->pos++;
            }

            ZYaml *val = z_yaml_parse_value(p);
            if (!val) { free(key); z_yaml_free(yaml); return NULL; }

            if (yaml->value.object.count >= yaml->value.object.capacity) {
                size_t new_cap = yaml->value.object.capacity == 0 ? 8 : yaml->value.object.capacity * 2;
                const char **new_keys = (const char **)realloc(yaml->value.object.keys, new_cap * sizeof(const char *));
                ZYaml **new_values = (ZYaml **)realloc(yaml->value.object.values, new_cap * sizeof(ZYaml *));
                if (!new_keys || !new_values) { free(key); z_yaml_free(val); z_yaml_free(yaml); return NULL; }
                yaml->value.object.keys = new_keys;
                yaml->value.object.values = new_values;
                yaml->value.object.capacity = new_cap;
            }
            yaml->value.object.keys[yaml->value.object.count] = key;
            yaml->value.object.values[yaml->value.object.count] = val;
            yaml->value.object.count++;

            z_yaml_skip_whitespace(p);
            if (p->pos < p->len && p->str[p->pos] == ',') {
                p->pos++;
                z_yaml_skip_whitespace(p);
            }
        }

        return yaml;
    }

    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_OBJECT;
    yaml->value.object.keys = NULL;
    yaml->value.object.values = NULL;
    yaml->value.object.count = 0;
    yaml->value.object.capacity = 0;

    while (p->pos < p->len) {
        z_yaml_skip_whitespace(p);

        if (p->pos >= p->len) break;

        if (p->str[p->pos] == '\n') {
            p->pos++;
            if (p->pos >= p->len) break;

            int indent = z_yaml_get_indent(p);
            if (indent < 0) continue;
            if (indent < base_indent) {
                break;
            }
            continue;
        }

        if (p->str[p->pos] == '#') {
            z_yaml_skip_line(p);
            continue;
        }

        int indent = z_yaml_get_indent(p);
        if (indent < 0) {
            z_yaml_skip_line(p);
            continue;
        }
        if (indent < base_indent) {
            break;
        }

        size_t key_start = p->pos;
        while (p->pos < p->len && p->str[p->pos] != ':' && !z_yaml_is_newline(p->str[p->pos])) {
            p->pos++;
        }

        if (p->pos >= p->len || p->str[p->pos] != ':') {
            if (p->pos < p->len && p->str[p->pos] == '-' && (p->pos + 1 < p->len && p->str[p->pos + 1] == ' ')) {
                ZYaml *arr = z_yaml_parse_array(p, base_indent);
                if (!arr) { z_yaml_free(yaml); return NULL; }
                if (yaml->value.object.count > 0) {
                    z_yaml_free(yaml->value.object.values[yaml->value.object.count - 1]);
                    yaml->value.object.values[yaml->value.object.count - 1] = arr;
                }
                continue;
            }
            p->pos = key_start;
            break;
        }

        size_t key_len = p->pos - key_start;
        while (key_len > 0 && (p->str[key_start + key_len - 1] == ' ' || p->str[key_start + key_len - 1] == '\t')) {
            key_len--;
        }

        char *key = (char *)malloc(key_len + 1);
        if (!key) { z_yaml_free(yaml); return NULL; }
        memcpy(key, p->str + key_start, key_len);
        key[key_len] = '\0';

        p->pos++;
        z_yaml_skip_whitespace(p);

        ZYaml *val = NULL;
        if (p->pos < p->len && !z_yaml_is_newline(p->str[p->pos]) && p->str[p->pos] != '#') {
            val = z_yaml_parse_value(p);
        } else {
            val = z_yaml_create_null();
            while (p->pos < p->len && z_yaml_is_newline(p->str[p->pos])) {
                p->pos++;
            }
            if (p->pos >= p->len) {
                yaml->value.object.keys[yaml->value.object.count] = key;
                yaml->value.object.values[yaml->value.object.count] = val;
                yaml->value.object.count++;
                break;
            }

            int next_indent = z_yaml_get_indent(p);
            if (next_indent > indent) {
                size_t saved_pos = p->pos;
                while (p->pos < p->len && (p->str[p->pos] == ' ' || p->str[p->pos] == '\t')) p->pos++;
                bool is_array = (p->pos < p->len && p->str[p->pos] == '-' && (p->pos + 1 >= p->len || p->str[p->pos + 1] == ' '));
                p->pos = saved_pos;

                ZYaml *nested;
                if (is_array) {
                    nested = z_yaml_parse_array(p, indent);
                } else {
                    nested = z_yaml_parse_object(p, indent);
                }
                if (nested) {
                    z_yaml_free(val);
                    val = nested;
                }
            } else {
                p->pos = key_start;
                while (p->pos < p->len && p->str[p->pos] != '\n') p->pos++;
                if (p->pos < p->len) p->pos++;
                z_yaml_free(val);
                val = z_yaml_create_null();
            }
        }

        if (!val) { free(key); z_yaml_free(yaml); return NULL; }

        if (yaml->value.object.count >= yaml->value.object.capacity) {
            size_t new_cap = yaml->value.object.capacity == 0 ? 8 : yaml->value.object.capacity * 2;
            const char **new_keys = (const char **)realloc(yaml->value.object.keys, new_cap * sizeof(const char *));
            ZYaml **new_values = (ZYaml **)realloc(yaml->value.object.values, new_cap * sizeof(ZYaml *));
            if (!new_keys || !new_values) { free(key); z_yaml_free(val); z_yaml_free(yaml); return NULL; }
            yaml->value.object.keys = new_keys;
            yaml->value.object.values = new_values;
            yaml->value.object.capacity = new_cap;
        }
        yaml->value.object.keys[yaml->value.object.count] = key;
        yaml->value.object.values[yaml->value.object.count] = val;
        yaml->value.object.count++;
    }

    return yaml;
}

static ZYaml *z_yaml_parse_value(z_yaml_parser *p) {
    if (p->pos >= p->len) return NULL;

    z_yaml_skip_whitespace(p);
    if (p->pos >= p->len) return NULL;

    if (p->str[p->pos] == '\n') {
        p->pos++;
        return z_yaml_create_null();
    }

    if (p->str[p->pos] == '#') {
        z_yaml_skip_line(p);
        return z_yaml_create_null();
    }

    if (p->str[p->pos] == '[' || p->str[p->pos] == '{') {
        if (p->str[p->pos] == '[') {
            return z_yaml_parse_array(p, p->indent);
        } else {
            return z_yaml_parse_object(p, p->indent);
        }
    }

    if (p->str[p->pos] == '-' && (p->pos + 1 >= p->len || p->str[p->pos + 1] == ' ')) {
        return z_yaml_parse_array(p, p->indent);
    }

    size_t val_start = p->pos;
    while (p->pos < p->len && !z_yaml_is_newline(p->str[p->pos]) && p->str[p->pos] != '#') {
        p->pos++;
    }

    size_t val_len = p->pos - val_start;
    while (val_len > 0 && (p->str[val_start + val_len - 1] == ' ' || p->str[val_start + val_len - 1] == '\t')) {
        val_len--;
    }

    if (val_len == 0) {
        return z_yaml_create_null();
    }

    char *val = (char *)malloc(val_len + 1);
    if (!val) return NULL;
    memcpy(val, p->str + val_start, val_len);
    val[val_len] = '\0';

    if (strcmp(val, "null") == 0 || strcmp(val, "~") == 0 || strcmp(val, "Null") == 0 || strcmp(val, "NULL") == 0) {
        free(val);
        return z_yaml_create_null();
    }

    if (strcmp(val, "true") == 0 || strcmp(val, "True") == 0 || strcmp(val, "TRUE") == 0) {
        free(val);
        ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
        if (!yaml) return NULL;
        yaml->type = Z_YAML_BOOL;
        yaml->value.bool_val = true;
        return yaml;
    }

    if (strcmp(val, "false") == 0 || strcmp(val, "False") == 0 || strcmp(val, "FALSE") == 0) {
        free(val);
        ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
        if (!yaml) return NULL;
        yaml->type = Z_YAML_BOOL;
        yaml->value.bool_val = false;
        return yaml;
    }

    char *endptr;
    double num = strtod(val, &endptr);
    if (*endptr == '\0' && val_len > 0) {
        free(val);
        ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
        if (!yaml) return NULL;
        yaml->type = Z_YAML_NUMBER;
        yaml->value.num_val = num;
        return yaml;
    }

    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) { free(val); return NULL; }
    yaml->type = Z_YAML_STRING;
    yaml->value.str_val = val;
    return yaml;
}

ZYaml *z_yaml_parse_str(const char *str) {
    z_yaml_parser p;
    p.str = str;
    p.pos = 0;
    p.len = strlen(str);
    p.indent = 0;

    while (p.pos < p.len && (p.str[p.pos] == ' ' || p.str[p.pos] == '\t' || p.str[p.pos] == '\n' || p.str[p.pos] == '\r')) {
        if (p.str[p.pos] == '\n') {
            p.pos++;
            if (p.pos < p.len && (p.str[p.pos] == '-' || p.str[p.pos] == '?')) {
                break;
            }
            while (p.pos < p.len && (p.str[p.pos] == ' ' || p.str[p.pos] == '\t')) {
                p.pos++;
            }
            if (p.pos < p.len && p.str[p.pos] == ':') {
                break;
            }
        } else {
            p.pos++;
        }
    }

    p.indent = 0;
    return z_yaml_parse_object(&p, 0);
}

ZYaml *z_yaml_parse_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *str = (char *)malloc(len + 1);
    if (!str) { fclose(f); return NULL; }
    fread(str, 1, len, f);
    str[len] = '\0';
    fclose(f);

    ZYaml *result = z_yaml_parse_str(str);
    free(str);
    return result;
}

ZYaml *z_yaml_object_get(ZYaml *obj, const char *key) {
    if (!obj || obj->type != Z_YAML_OBJECT) return NULL;
    for (size_t i = 0; i < obj->value.object.count; i++) {
        if (strcmp(obj->value.object.keys[i], key) == 0) {
            return obj->value.object.values[i];
        }
    }
    return NULL;
}

ZYaml *z_yaml_array_get(ZYaml *arr, size_t index) {
    if (!arr || arr->type != Z_YAML_ARRAY) return NULL;
    if (index >= arr->value.array.count) return NULL;
    return arr->value.array.items[index];
}

ZYaml *z_yaml_get_path(ZYaml *yaml, const char *path) {
    if (!yaml || !path) return NULL;

    char *path_copy = strdup(path);
    if (!path_copy) return NULL;

    ZYaml *current = yaml;
    char *p = path_copy;
    char *token;

    while ((token = strtok(p, ".")) != NULL) {
        p = NULL;

        char *bracket = strchr(token, '[');
        if (bracket) {
            *bracket = '\0';
            char *end = strchr(bracket + 1, ']');
            if (end) *end = '\0';

            if (strlen(token) > 0) {
                current = z_yaml_object_get(current, token);
                if (!current) { free(path_copy); return NULL; }
            }

            if (strlen(bracket + 1) > 0) {
                int index = atoi(bracket + 1);
                current = z_yaml_array_get(current, (size_t)index);
                if (!current) { free(path_copy); return NULL; }
            }
        } else {
            current = z_yaml_object_get(current, token);
            if (!current) { free(path_copy); return NULL; }
        }
    }

    free(path_copy);
    return current;
}

size_t z_yaml_array_size(ZYaml *arr) {
    if (!arr || arr->type != Z_YAML_ARRAY) return 0;
    return arr->value.array.count;
}

static void z_yaml_escape_string(const char *str, char **buf, size_t *pos, size_t *cap) {
    while (*str) {
        if (*pos + 2 >= *cap) {
            *cap = *cap * 2;
            *buf = (char *)realloc(*buf, *cap);
        }
        if (*str == '\n') {
            (*buf)[(*pos)++] = '\\';
            (*buf)[(*pos)++] = 'n';
        } else if (*str == '\r') {
            (*buf)[(*pos)++] = '\\';
            (*buf)[(*pos)++] = 'r';
        } else if (*str == '\t') {
            (*buf)[(*pos)++] = '\\';
            (*buf)[(*pos)++] = 't';
        } else if (*str == '"' || *str == '\\') {
            (*buf)[(*pos)++] = '\\';
            (*buf)[(*pos)++] = *str;
        } else if ((unsigned char)*str < 32) {
            char hex[4];
            snprintf(hex, sizeof(hex), "\\x%02x", (unsigned char)*str);
            for (int i = 0; hex[i]; i++) {
                if (*pos + 1 >= *cap) {
                    *cap = *cap * 2;
                    *buf = (char *)realloc(*buf, *cap);
                }
                (*buf)[(*pos)++] = hex[i];
            }
        } else {
            (*buf)[(*pos)++] = *str;
        }
        str++;
    }
}

static void z_yaml_stringify_value(ZYaml *yaml, char **buf, size_t *pos, size_t *cap) {
    if (!yaml) return;

    if (*pos + 64 >= *cap) {
        *cap = *cap == 0 ? 256 : *cap * 2;
        *buf = (char *)realloc(*buf, *cap);
    }

    switch (yaml->type) {
        case Z_YAML_NULL:
            memcpy(*buf + *pos, "null", 4);
            *pos += 4;
            break;
        case Z_YAML_BOOL:
            if (yaml->value.bool_val) {
                memcpy(*buf + *pos, "true", 4);
                *pos += 4;
            } else {
                memcpy(*buf + *pos, "false", 5);
                *pos += 5;
            }
            break;
        case Z_YAML_NUMBER: {
            char num_str[64];
            snprintf(num_str, sizeof(num_str), "%.15g", yaml->value.num_val);
            size_t num_len = strlen(num_str);
            if (*pos + num_len >= *cap) {
                *cap = *cap * 2;
                *buf = (char *)realloc(*buf, *cap);
            }
            memcpy(*buf + *pos, num_str, num_len);
            *pos += num_len;
            break;
        }
        case Z_YAML_STRING: {
            bool need_quote = false;
            const char *s = yaml->value.str_val;
            while (*s) {
                if (*s == ':' || *s == '#' || *s == '[' || *s == ']' || *s == '{' || *s == '}' ||
                    *s == ',' || *s == '&' || *s == '*' || *s == '!' || *s == '|' || *s == '>' ||
                    *s == '\'' || *s == '"' || *s == '%' || *s == '@' || *s == '`' ||
                    *s == '\n' || *s == '\r' || *s == '\t' || *s == ' ' || *s == '-') {
                    need_quote = true;
                    break;
                }
                s++;
            }

            if (need_quote) {
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                z_yaml_escape_string(yaml->value.str_val, buf, pos, cap);
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
            } else {
                s = yaml->value.str_val;
                while (*s) {
                    if (*pos + 1 >= *cap) {
                        *cap = *cap * 2;
                        *buf = (char *)realloc(*buf, *cap);
                    }
                    (*buf)[(*pos)++] = *s++;
                }
            }
            break;
        }
        case Z_YAML_ARRAY: {
            if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
            (*buf)[(*pos)++] = '[';
            for (size_t i = 0; i < yaml->value.array.count; i++) {
                if (i > 0) {
                    if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    (*buf)[(*pos)++] = ',';
                }
                z_yaml_stringify_value(yaml->value.array.items[i], buf, pos, cap);
            }
            if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
            (*buf)[(*pos)++] = ']';
            break;
        }
        case Z_YAML_OBJECT: {
            if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
            (*buf)[(*pos)++] = '{';
            for (size_t i = 0; i < yaml->value.object.count; i++) {
                if (i > 0) {
                    if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    (*buf)[(*pos)++] = ',';
                }
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                const char *k = yaml->value.object.keys[i];
                while (*k) {
                    if (*pos + 1 >= *cap) {
                        *cap = *cap * 2;
                        *buf = (char *)realloc(*buf, *cap);
                    }
                    (*buf)[(*pos)++] = *k++;
                }
                if (*pos + 3 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                (*buf)[(*pos)++] = ':';
                z_yaml_stringify_value(yaml->value.object.values[i], buf, pos, cap);
            }
            if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
            (*buf)[(*pos)++] = '}';
            break;
        }
    }
}

char *z_yaml_stringify(ZYaml *yaml, size_t *out_len) {
    if (!yaml) { if (out_len) *out_len = 0; return NULL; }
    size_t pos = 0, cap = 256;
    char *buf = (char *)malloc(cap);
    if (!buf) return NULL;
    buf[0] = '\0';

    z_yaml_stringify_value(yaml, &buf, &pos, &cap);
    if (pos + 1 >= cap) {
        buf = (char *)realloc(buf, pos + 1);
    }
    buf[pos] = '\0';
    if (out_len) *out_len = pos;
    return buf;
}

static void z_yaml_add_indent(char **buf, size_t *pos, size_t *cap, int level, const char *indent_str) {
    if (*pos + 1 >= *cap) {
        *cap = *cap * 2;
        *buf = (char *)realloc(*buf, *cap);
    }
    (*buf)[(*pos)++] = '\n';
    (*buf)[*pos] = '\0';

    for (int i = 0; i < level; i++) {
        size_t indent_len = strlen(indent_str);
        if (*pos + indent_len >= *cap) {
            *cap = *cap * 2;
            *buf = (char *)realloc(*buf, *cap);
        }
        strncat(*buf, indent_str, indent_len);
        *pos += indent_len;
    }
}

static void z_yaml_stringify_value_pretty(ZYaml *yaml, char **buf, size_t *pos, size_t *cap, int indent_level, const char *indent_str, bool is_flow) {
    if (!yaml) return;

    if (is_flow) {
        switch (yaml->type) {
            case Z_YAML_NULL:
                if (*pos + 4 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                strncat(*buf, "null", 4);
                *pos += 4;
                break;
            case Z_YAML_BOOL:
                if (yaml->value.bool_val) {
                    if (*pos + 4 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    strncat(*buf, "true", 4);
                    *pos += 4;
                } else {
                    if (*pos + 5 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    strncat(*buf, "false", 5);
                    *pos += 5;
                }
                break;
            case Z_YAML_NUMBER: {
                char num_str[64];
                snprintf(num_str, sizeof(num_str), "%.15g", yaml->value.num_val);
                size_t num_len = strlen(num_str);
                if (*pos + num_len >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                strncat(*buf, num_str, num_len);
                *pos += num_len;
                break;
            }
            case Z_YAML_STRING: {
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                (*buf)[*pos] = '\0';
                z_yaml_escape_string(yaml->value.str_val, buf, pos, cap);
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                (*buf)[*pos] = '\0';
                break;
            }
            case Z_YAML_ARRAY: {
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '[';
                (*buf)[*pos] = '\0';
                for (size_t i = 0; i < yaml->value.array.count; i++) {
                    if (i > 0) { strcat(*buf, ","); *pos += 1; }
                    z_yaml_stringify_value_pretty(yaml->value.array.items[i], buf, pos, cap, indent_level, indent_str, true);
                }
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = ']';
                (*buf)[*pos] = '\0';
                break;
            }
            case Z_YAML_OBJECT: {
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '{';
                (*buf)[*pos] = '\0';
                for (size_t i = 0; i < yaml->value.object.count; i++) {
                    if (i > 0) { strcat(*buf, ","); *pos += 1; }
                    strcat(*buf, "\"");
                    *pos += 1;
                    const char *k = yaml->value.object.keys[i];
                    while (*k) {
                        if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                        (*buf)[(*pos)++] = *k++;
                    }
                    if (*pos + 3 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    (*buf)[(*pos)++] = '"';
                    (*buf)[(*pos)++] = ':';
                    (*buf)[(*pos)++] = ' ';
                    (*buf)[*pos] = '\0';
                    z_yaml_stringify_value_pretty(yaml->value.object.values[i], buf, pos, cap, indent_level, indent_str, true);
                }
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '}';
                (*buf)[*pos] = '\0';
                break;
            }
        }
        return;
    }

    switch (yaml->type) {
        case Z_YAML_NULL:
            if (*pos + 4 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
            strncat(*buf, "null", 4);
            *pos += 4;
            break;
        case Z_YAML_BOOL:
            if (yaml->value.bool_val) {
                if (*pos + 4 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                strncat(*buf, "true", 4);
                *pos += 4;
            } else {
                if (*pos + 5 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                strncat(*buf, "false", 5);
                *pos += 5;
            }
            break;
        case Z_YAML_NUMBER: {
            char num_str[64];
            snprintf(num_str, sizeof(num_str), "%.15g", yaml->value.num_val);
            size_t num_len = strlen(num_str);
            if (*pos + num_len >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
            strncat(*buf, num_str, num_len);
            *pos += num_len;
            break;
        }
        case Z_YAML_STRING: {
            bool need_quote = false;
            const char *s = yaml->value.str_val;
            while (*s) {
                if (*s == ':' || *s == '#' || *s == '[' || *s == ']' || *s == '{' || *s == '}' ||
                    *s == ',' || *s == '&' || *s == '*' || *s == '!' || *s == '|' || *s == '>' ||
                    *s == '\'' || *s == '"' || *s == '%' || *s == '@' || *s == '`' ||
                    *s == '\n' || *s == '\r' || *s == '\t' || *s == ' ' || *s == '-') {
                    need_quote = true;
                    break;
                }
                s++;
            }

            if (need_quote) {
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                (*buf)[*pos] = '\0';
                z_yaml_escape_string(yaml->value.str_val, buf, pos, cap);
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '"';
                (*buf)[*pos] = '\0';
            } else {
                s = yaml->value.str_val;
                while (*s) {
                    if (*pos + 1 >= *cap) {
                        *cap = *cap * 2;
                        *buf = (char *)realloc(*buf, *cap);
                    }
                    (*buf)[(*pos)++] = *s++;
                }
            }
            break;
        }
        case Z_YAML_ARRAY: {
            for (size_t i = 0; i < yaml->value.array.count; i++) {
                if (i > 0) {
                    z_yaml_add_indent(buf, pos, cap, indent_level, indent_str);
                } else {
                    z_yaml_add_indent(buf, pos, cap, indent_level, indent_str);
                }
                if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                (*buf)[(*pos)++] = '-';
                (*buf)[(*pos)++] = ' ';
                (*buf)[*pos] = '\0';

                if (yaml->value.array.items[i]->type == Z_YAML_OBJECT || yaml->value.array.items[i]->type == Z_YAML_ARRAY) {
                    z_yaml_stringify_value_pretty(yaml->value.array.items[i], buf, pos, cap, indent_level + 1, indent_str, true);
                } else {
                    z_yaml_stringify_value_pretty(yaml->value.array.items[i], buf, pos, cap, indent_level, indent_str, false);
                }
            }
            break;
        }
        case Z_YAML_OBJECT: {
            for (size_t i = 0; i < yaml->value.object.count; i++) {
                z_yaml_add_indent(buf, pos, cap, indent_level, indent_str);

                const char *k = yaml->value.object.keys[i];
                while (*k) {
                    if (*pos + 1 >= *cap) {
                        *cap = *cap * 2;
                        *buf = (char *)realloc(*buf, *cap);
                    }
                    (*buf)[(*pos)++] = *k++;
                }
                (*buf)[*pos] = '\0';

                ZYaml *v = yaml->value.object.values[i];
                if (v->type == Z_YAML_OBJECT || v->type == Z_YAML_ARRAY) {
                    if (*pos + 1 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    (*buf)[(*pos)++] = ':';
                    (*buf)[*pos] = '\0';
                    if (v->type == Z_YAML_ARRAY && v->value.array.count == 0) {
                        strcat(*buf, " []");
                        *pos += 3;
                    } else if (v->type == Z_YAML_OBJECT && v->value.object.count == 0) {
                        strcat(*buf, " {}");
                        *pos += 3;
                    } else {
                        z_yaml_stringify_value_pretty(v, buf, pos, cap, indent_level + 1, indent_str, false);
                    }
                } else {
                    if (*pos + 2 >= *cap) { *cap = *cap * 2; *buf = (char *)realloc(*buf, *cap); }
                    (*buf)[(*pos)++] = ':';
                    (*buf)[(*pos)++] = ' ';
                    (*buf)[*pos] = '\0';
                    z_yaml_stringify_value_pretty(v, buf, pos, cap, indent_level, indent_str, false);
                }
            }
            break;
        }
    }
}

char *z_yaml_stringify_pretty(ZYaml *yaml, size_t *out_len, int indent) {
    if (!yaml) { if (out_len) *out_len = 0; return NULL; }
    if (indent <= 0) return z_yaml_stringify(yaml, out_len);

    char *indent_str = (char *)malloc(indent + 1);
    if (!indent_str) return NULL;
    for (int i = 0; i < indent; i++) indent_str[i] = ' ';
    indent_str[indent] = '\0';

    size_t pos = 0, cap = 256;
    char *buf = (char *)malloc(cap);
    if (!buf) { free(indent_str); return NULL; }
    buf[0] = '\0';

    z_yaml_stringify_value_pretty(yaml, &buf, &pos, &cap, 0, indent_str, false);

    free(indent_str);
    if (out_len) *out_len = pos;
    return buf;
}

ZYaml *z_yaml_create_null(void) {
    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_NULL;
    return yaml;
}

ZYaml *z_yaml_create_string(const char *str) {
    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_STRING;
    yaml->value.str_val = str ? strdup(str) : NULL;
    return yaml;
}

ZYaml *z_yaml_create_number(double num) {
    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_NUMBER;
    yaml->value.num_val = num;
    return yaml;
}

ZYaml *z_yaml_create_bool(bool val) {
    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_BOOL;
    yaml->value.bool_val = val;
    return yaml;
}

ZYaml *z_yaml_create_object(void) {
    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_OBJECT;
    yaml->value.object.keys = NULL;
    yaml->value.object.values = NULL;
    yaml->value.object.count = 0;
    yaml->value.object.capacity = 0;
    return yaml;
}

ZYaml *z_yaml_create_array(void) {
    ZYaml *yaml = (ZYaml *)malloc(sizeof(ZYaml));
    if (!yaml) return NULL;
    yaml->type = Z_YAML_ARRAY;
    yaml->value.array.items = NULL;
    yaml->value.array.count = 0;
    yaml->value.array.capacity = 0;
    return yaml;
}

static int z_yaml_object_find_index(ZYaml *obj, const char *key) {
    for (size_t i = 0; i < obj->value.object.count; i++) {
        if (strcmp(obj->value.object.keys[i], key) == 0) {
            return (int)i;
        }
    }
    return -1;
}

bool z_yaml_object_set(ZYaml *obj, const char *key, ZYaml *value) {
    if (!obj || obj->type != Z_YAML_OBJECT || !key || !value) return false;

    int idx = z_yaml_object_find_index(obj, key);
    if (idx >= 0) {
        z_yaml_free(obj->value.object.values[idx]);
        obj->value.object.values[idx] = value;
        return true;
    }

    if (obj->value.object.count >= obj->value.object.capacity) {
        size_t new_cap = obj->value.object.capacity == 0 ? 8 : obj->value.object.capacity * 2;
        const char **new_keys = (const char **)realloc(obj->value.object.keys, new_cap * sizeof(const char *));
        ZYaml **new_values = (ZYaml **)realloc(obj->value.object.values, new_cap * sizeof(ZYaml *));
        if (!new_keys || !new_values) return false;
        obj->value.object.keys = new_keys;
        obj->value.object.values = new_values;
        obj->value.object.capacity = new_cap;
    }

    char *key_dup = strdup(key);
    if (!key_dup) return false;

    obj->value.object.keys[obj->value.object.count] = key_dup;
    obj->value.object.values[obj->value.object.count] = value;
    obj->value.object.count++;
    return true;
}

bool z_yaml_object_remove(ZYaml *obj, const char *key) {
    if (!obj || obj->type != Z_YAML_OBJECT || !key) return false;

    int idx = z_yaml_object_find_index(obj, key);
    if (idx < 0) return false;

    free((void *)obj->value.object.keys[idx]);
    z_yaml_free(obj->value.object.values[idx]);

    for (size_t i = idx; i < obj->value.object.count - 1; i++) {
        obj->value.object.keys[i] = obj->value.object.keys[i + 1];
        obj->value.object.values[i] = obj->value.object.values[i + 1];
    }
    obj->value.object.count--;
    return true;
}

bool z_yaml_array_append(ZYaml *arr, ZYaml *value) {
    if (!arr || arr->type != Z_YAML_ARRAY || !value) return false;

    if (arr->value.array.count >= arr->value.array.capacity) {
        size_t new_cap = arr->value.array.capacity == 0 ? 8 : arr->value.array.capacity * 2;
        ZYaml **new_items = (ZYaml **)realloc(arr->value.array.items, new_cap * sizeof(ZYaml *));
        if (!new_items) return false;
        arr->value.array.items = new_items;
        arr->value.array.capacity = new_cap;
    }

    arr->value.array.items[arr->value.array.count++] = value;
    return true;
}

bool z_yaml_array_remove(ZYaml *arr, size_t index) {
    if (!arr || arr->type != Z_YAML_ARRAY || index >= arr->value.array.count) return false;

    z_yaml_free(arr->value.array.items[index]);

    for (size_t i = index; i < arr->value.array.count - 1; i++) {
        arr->value.array.items[i] = arr->value.array.items[i + 1];
    }
    arr->value.array.count--;
    return true;
}

bool z_yaml_write_file(ZYaml *yaml, const char *filename) {
    if (!yaml || !filename) return false;

    size_t len;
    char *str = z_yaml_stringify(yaml, &len);
    if (!str) return false;

    FILE *f = fopen(filename, "wb");
    if (!f) { free(str); return false; }

    size_t written = fwrite(str, 1, len, f);
    fclose(f);
    free(str);

    return written == len;
}

bool z_yaml_write_file_pretty(ZYaml *yaml, const char *filename, int indent) {
    if (!yaml || !filename) return false;

    size_t len;
    char *str = z_yaml_stringify_pretty(yaml, &len, indent);
    if (!str) return false;

    FILE *f = fopen(filename, "wb");
    if (!f) { free(str); return false; }

    size_t written = fwrite(str, 1, len, f);
    fclose(f);
    free(str);

    return written == len;
}

void z_yaml_free(ZYaml *yaml) {
    if (!yaml) return;

    switch (yaml->type) {
        case Z_YAML_STRING:
            free((void *)yaml->value.str_val);
            break;
        case Z_YAML_OBJECT:
            for (size_t i = 0; i < yaml->value.object.count; i++) {
                free((void *)yaml->value.object.keys[i]);
                z_yaml_free(yaml->value.object.values[i]);
            }
            free(yaml->value.object.keys);
            free(yaml->value.object.values);
            break;
        case Z_YAML_ARRAY:
            for (size_t i = 0; i < yaml->value.array.count; i++) {
                z_yaml_free(yaml->value.array.items[i]);
            }
            free(yaml->value.array.items);
            break;
        default: break;
    }
    free(yaml);
}

#endif /* Z_YAML_IMPLEMENTATION */
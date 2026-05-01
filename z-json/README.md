# z-json

一个轻量级的 C 语言 JSON 解析器和构建器，采用 stb-style 单头文件设计。

## 特性

- 解析 JSON 字符串和文件
- 构建和修改 JSON 数据
- 支持格式化输出
- 支持路径查询（如 `a.b[0].c`）
- 无外部依赖

## 集成方式

z-json 使用 stb-style 单头文件设计。在**一个** C 文件中定义 `Z_JSON_IMPLEMENTATION` 后再包含头文件：

```c
#define Z_JSON_IMPLEMENTATION
#include "z-json.h"
```

其他文件只需正常包含即可：

```c
#include "z-json.h"
```

## API 说明

### 数据类型

```c
typedef enum {
    Z_JSON_NULL,
    Z_JSON_STRING,
    Z_JSON_NUMBER,
    Z_JSON_BOOL,
    Z_JSON_OBJECT,
    Z_JSON_ARRAY
} Z_JSON_TYPE;

typedef struct ZJson {
    Z_JSON_TYPE type;
    union {
        const char *str_val;
        double num_val;
        bool bool_val;
        struct {
            struct ZJson **items;
            size_t count;
            size_t capacity;
        } array;
        struct {
            const char **keys;
            struct ZJson **values;
            size_t count;
            size_t capacity;
        } object;
    } value;
} ZJson;
```

### 解析函数

```c
// 从字符串解析 JSON
ZJson *z_json_parse_str(const char *str);

// 从文件解析 JSON
ZJson *z_json_parse_file(const char *filename);
```

### 创建函数

```c
ZJson *z_json_create_null(void);
ZJson *z_json_create_string(const char *str);
ZJson *z_json_create_number(double num);
ZJson *z_json_create_bool(bool val);
ZJson *z_json_create_object(void);
ZJson *z_json_create_array(void);
```

### 修改函数

```c
// 对象操作
bool z_json_object_set(ZJson *obj, const char *key, ZJson *value);
bool z_json_object_remove(ZJson *obj, const char *key);
ZJson *z_json_object_get(ZJson *obj, const char *key);

// 数组操作
bool z_json_array_append(ZJson *arr, ZJson *value);
bool z_json_array_remove(ZJson *arr, size_t index);
ZJson *z_json_array_get(ZJson *arr, size_t index);
size_t z_json_array_size(ZJson *arr);
```

### 查询函数

```c
// 路径查询，支持点号和数组索引，如 "a.b[0].c"
ZJson *z_json_get_path(ZJson *json, const char *path);
```

### 序列化函数

```c
// 普通序列化
char *z_json_stringify(ZJson *json, size_t *out_len);

// 格式化序列化，indent 为缩进空格数
char *z_json_stringify_pretty(ZJson *json, size_t *out_len, int indent);

// 默认缩进（4 空格）
#define Z_JSON_DEFAULT_INDENT 4
```

### 文件写入函数

```c
// 写入文件（紧凑格式）
bool z_json_write_file(ZJson *json, const char *filename);

// 写入文件（格式化）
bool z_json_write_file_pretty(ZJson *json, const char *filename, int indent);
```

### 释放函数

```c
void z_json_free(ZJson *json);
```

## 使用示例

### 解析 JSON

```c
#define Z_JSON_IMPLEMENTATION
#include "z-json.h"
#include <stdio.h>

int main() {
    const char *json_str = "{\"name\": \"z-json\", \"version\": 1.0}";
    
    ZJson *root = z_json_parse_str(json_str);
    if (!root) {
        printf("Failed to parse JSON\n");
        return 1;
    }
    
    ZJson *name = z_json_object_get(root, "name");
    if (name && name->type == Z_JSON_STRING) {
        printf("name: %s\n", name->value.str_val);
    }
    
    z_json_free(root);
    return 0;
}
```

### 构建 JSON

```c
#define Z_JSON_IMPLEMENTATION
#include "z-json.h"
#include <stdio.h>

int main() {
    ZJson *obj = z_json_create_object();
    z_json_object_set(obj, "name", z_json_create_string("test"));
    z_json_object_set(obj, "version", z_json_create_number(2.5));
    z_json_object_set(obj, "active", z_json_create_bool(true));
    
    ZJson *arr = z_json_create_array();
    z_json_array_append(arr, z_json_create_string("a"));
    z_json_array_append(arr, z_json_create_number(1));
    z_json_object_set(obj, "items", arr);
    
    // 序列化输出
    size_t len;
    char *str = z_json_stringify(obj, &len);
    printf("JSON: %s\n", str);
    free(str);
    
    // 格式化输出
    char *pretty = z_json_stringify_pretty(obj, &len, 2);
    printf("Pretty:\n%s\n", pretty);
    free(pretty);
    
    // 写入文件
    z_json_write_file_pretty(obj, "output.json", Z_JSON_DEFAULT_INDENT);
    
    z_json_free(obj);
    return 0;
}
```

### 路径查询

```c
ZJson *root = z_json_parse_str("{\"a\": {\"b\": [1, 2, 3]}}");

// 查询 a.b[1]
ZJson *val = z_json_get_path(root, "a.b[1]");
if (val && val->type == Z_JSON_NUMBER) {
    printf("a.b[1] = %g\n", val->value.num_val);  // 输出: 2
}
```

## 编译

```bash
gcc -o example example.c -Wall
```

## 许可证

MIT License

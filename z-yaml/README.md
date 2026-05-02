# z-yaml

一个轻量级的 C 语言 YAML 解析器和构建器，采用 stb-style 单头文件设计。

## 特性

- 解析 YAML 字符串和文件
- 构建和修改 YAML 数据
- 支持格式化输出
- 无外部依赖

## 集成方式

z-yaml 使用 stb-style 单头文件设计。在**一个** C 文件中定义 `Z_YAML_IMPLEMENTATION` 后再包含头文件：

```c
#define Z_YAML_IMPLEMENTATION
#include "z-yaml.h"
```

其他文件只需正常包含即可：

```c
#include "z-yaml.h"
```

## API 说明

### 数据类型

```c
typedef enum {
    Z_YAML_NULL,
    Z_YAML_STRING,
    Z_YAML_NUMBER,
    Z_YAML_BOOL,
    Z_YAML_OBJECT,
    Z_YAML_ARRAY
} ZYamlType;
```

### 解析函数

```c
// 从字符串解析 YAML
ZYaml *z_yaml_parse_str(const char *str);

// 从文件解析 YAML
ZYaml *z_yaml_parse_file(const char *filename);
```

### 创建函数

```c
ZYaml *z_yaml_create_null(void);
ZYaml *z_yaml_create_string(const char *str);
ZYaml *z_yaml_create_number(double num);
ZYaml *z_yaml_create_bool(bool val);
ZYaml *z_yaml_create_object(void);
ZYaml *z_yaml_create_array(void);
```

### 对象操作

```c
bool z_yaml_object_set(ZYaml *obj, const char *key, ZYaml *value);
bool z_yaml_object_remove(ZYaml *obj, const char *key);
ZYaml *z_yaml_object_get(ZYaml *obj, const char *key);
```

### 数组操作

```c
bool z_yaml_array_append(ZYaml *arr, ZYaml *value);
bool z_yaml_array_remove(ZYaml *arr, size_t index);
ZYaml *z_yaml_array_get(ZYaml *arr, size_t index);
size_t z_yaml_array_size(ZYaml *arr);
```

### 文件写入

```c
// 写入文件，indent 为缩进空格数
bool z_yaml_write_file(ZYaml *yaml, const char *filename, int indent);

// 默认缩进
#define Z_YAML_DEFAULT_INDENT 2
```

### 释放函数

```c
void z_yaml_free(ZYaml *yaml);
```

## 使用示例

### 解析 YAML

```c
#define Z_YAML_IMPLEMENTATION
#include "z-yaml.h"
#include <stdio.h>

int main() {
    const char *yaml_str = "name: z-yaml\nversion: 1.0\n";
    
    ZYaml *root = z_yaml_parse_str(yaml_str);
    if (!root) {
        printf("Failed to parse YAML\n");
        return 1;
    }
    
    ZYaml *name = z_yaml_object_get(root, "name");
    if (name && name->type == Z_YAML_STRING) {
        printf("name: %s\n", name->value.str_val);
    }
    
    z_yaml_free(root);
    return 0;
}
```

### 构建 YAML

```c
#define Z_YAML_IMPLEMENTATION
#include "z-yaml.h"
#include <stdio.h>

int main() {
    ZYaml *obj = z_yaml_create_object();
    z_yaml_object_set(obj, "name", z_yaml_create_string("test"));
    z_yaml_object_set(obj, "version", z_yaml_create_number(2.5));
    
    ZYaml *arr = z_yaml_create_array();
    z_yaml_array_append(arr, z_yaml_create_string("a"));
    z_yaml_array_append(arr, z_yaml_create_number(1));
    z_yaml_object_set(obj, "items", arr);
    
    z_yaml_write_file(obj, "output.yaml", Z_YAML_DEFAULT_INDENT);
    
    z_yaml_free(obj);
    return 0;
}
```

## 编译

```bash
gcc -o example example.c -Wall
```

## 许可证

MIT License
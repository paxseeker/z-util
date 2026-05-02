# z-containers

一个轻量级的 C 语言容器库，提供动态数组和哈希表，采用 stb-style 单头文件设计。

## 特性

- **z_vector**: 动态数组，支持任意类型
- **z_map**: 哈希表，支持键值对存储
- 无外部依赖
- 简单易用的 API

## 集成方式

z-containers 使用 stb-style 单头文件设计。在**一个** C 文件中定义 `Z_CONTAINERS_IMPLEMENTATION` 后再包含头文件：

```c
#define Z_CONTAINERS_IMPLEMENTATION
#include "z-containers.h"
```

其他文件只需正常包含即可：

```c
#include "z-containers.h"
```

## API 说明

### z_vector

```c
// 创建向量
z_vector z_vector_create(size_t item_size);

// 释放向量
void z_vector_free(z_vector *v);

// 添加元素
void *z_vector_push(z_vector *v, const void *item);

// 获取元素
void *z_vector_get(z_vector *v, size_t index);

// 设置元素
void z_vector_set(z_vector *v, size_t index, const void *item);

// 弹出最后一个元素
void z_vector_pop(z_vector *v, void *out);

// 获取元素数量
size_t z_vector_count(z_vector *v);
```

### z_map

```c
// 创建哈希表
z_map z_map_create(size_t key_size, size_t value_size);

// 释放哈希表
void z_map_free(z_map *m);

// 插入或更新键值对
bool z_map_set(z_map *m, const void *key, const void *value);

// 获取值
void *z_map_get(z_map *m, const void *key);

// 删除键值对
bool z_map_remove(z_map *m, const void *key);

// 获取元素数量
size_t z_map_count(z_map *m);
```

## 使用示例

### z_vector

```c
#define Z_CONTAINERS_IMPLEMENTATION
#include "z-containers.h"
#include <stdio.h>

int main() {
    z_vector v = z_vector_create(sizeof(int));

    int a = 10, b = 20, c = 30;
    z_vector_push(&v, &a);
    z_vector_push(&v, &b);
    z_vector_push(&v, &c);

    for (size_t i = 0; i < z_vector_count(&v); i++) {
        int *val = z_vector_get(&v, i);
        printf("%d ", *val);
    }
    printf("\n");

    z_vector_free(&v);
    return 0;
}
```

### z_map

```c
#define Z_CONTAINERS_IMPLEMENTATION
#include "z-containers.h"
#include <stdio.h>
#include <string.h>

int main() {
    z_map m = z_map_create(sizeof(char*), sizeof(int));

    const char *key1 = "apple";
    int val1 = 5;
    z_map_set(&m, &key1, &val1);

    const char *key2 = "banana";
    int val2 = 3;
    z_map_set(&m, &key2, &val2);

    int *found = z_map_get(&m, &key1);
    if (found) printf("apple: %d\n", *found);

    z_map_free(&m);
    return 0;
}
```

## 编译

```bash
gcc -o example example.c -Wall
```

## 许可证

MIT License
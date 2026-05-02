# z-cli

一个轻量级的 C 语言命令行参数解析库，采用 stb-style 单头文件设计。

## 特性

- 支持短选项（如 `-v`）和长选项（如 `--version`）
- 支持位置参数
- 支持多种类型：字符串、整数、浮点数、布尔值
- 自动生成帮助信息
- 无外部依赖

## 集成方式

z-cli 使用 stb-style 单头文件设计。在**一个** C 文件中定义 `Z_CLI_IMPLEMENTATION` 后再包含头文件：

```c
#define Z_CLI_IMPLEMENTATION
#include "z-cli.h"
```

其他文件只需正常包含即可：

```c
#include "z-cli.h"
```

## API 说明

### 初始化

```c
void z_cli_init(z_cli_t *cli, const char *program_name, const char *version);
void z_cli_set_description(z_cli_t *cli, const char *description);
```

### 添加参数

```c
// 添加布尔标志
void z_cli_add_flag(z_cli_t *cli, char short_name, const char *long_name, const char *description, bool *value);

// 添加选项（需要值的参数）
void z_cli_add_option(z_cli_t *cli, char short_name, const char *long_name, const char *description, z_cli_type type, void *value);

// 添加位置参数
void z_cli_add_positional(z_cli_t *cli, const char *name, const char *description, z_cli_type type, void *value);
```

### 解析和输出

```c
// 解析命令行参数
int z_cli_parse(z_cli_t *cli, int argc, char **argv);

// 打印帮助信息
void z_cli_print_help(z_cli_t *cli);

// 清理资源
void z_cli_destroy(z_cli_t *cli);
```

## 使用示例

```c
#define Z_CLI_IMPLEMENTATION
#include "z-cli.h"
#include <stdio.h>

int main(int argc, char **argv) {
    z_cli_t cli;
    z_cli_init(&cli, "myapp", "1.0.0");
    z_cli_set_description(&cli, "A sample application");

    bool verbose = false;
    char *output = NULL;
    int count = 1;

    z_cli_add_flag(&cli, 'v', "verbose", "Enable verbose output", &verbose);
    z_cli_add_option(&cli, 'o', "output", "Output file", Z_CLI_STRING, &output);
    z_cli_add_option(&cli, 'c', "count", "Count", Z_CLI_INT, &count);
    z_cli_add_positional(&cli, "input", "Input file", Z_CLI_STRING, &input);

    int ret = z_cli_parse(&cli, argc, argv);
    if (ret != 0) {
        if (ret == -1) z_cli_print_help(&cli);
        return 1;
    }

    printf("verbose: %s\n", verbose ? "true" : "false");
    printf("output: %s\n", output ? output : "(null)");
    printf("count: %d\n", count);
    printf("input: %s\n", input ? input : "(null)");

    z_cli_destroy(&cli);
    return 0;
}
```

## 编译

```bash
gcc -o example example.c -Wall
```

## 许可证

MIT License
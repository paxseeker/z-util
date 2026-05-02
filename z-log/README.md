# z-log

一个轻量级的 C 语言日志库，支持多级别日志和异步写入。

## 特性

- 多级别日志：DEBUG、INFO、WARN、ERROR
- 异步写入，避免阻塞主线程
- 支持输出到文件（通过定义 `LOG_FILE_PATH`）
- 线程安全
- 支持自定义队列大小（通过定义 `QUEUE_SIZE`）

## 集成方式

在**一个** C 文件中定义 `Z_LOG_IMPLEMENTATION` 后再包含头文件：

```c
#define Z_LOG_IMPLEMENTATION
#include "z-log.h"
```

其他文件只需正常包含即可：

```c
#include "z-log.h"
```

如需输出到文件，定义 `LOG_FILE_PATH`：

```c
#define LOG_FILE_PATH "app.log"
#define Z_LOG_IMPLEMENTATION
#include "z-log.h"
```

## API 说明

### 初始化和退出

```c
int z_log_init(void);
int z_log_exit(void);
```

### 日志宏

```c
INFO(x, ...)      // 信息日志
DEBUG(x, ...)     // 调试日志
WARN(x, ...)      // 警告日志
ERROR(x, ...)     // 错误日志
```

### 级别定义

```c
#define Z_LOG_DEBUG 0
#define Z_LOG_INFO 1
#define Z_LOG_WARN 2
#define Z_LOG_ERROR 3
```

## 使用示例

```c
#define Z_LOG_IMPLEMENTATION
#include "z-log.h"

int main() {
    z_log_init();

    INFO("Application started");
    DEBUG("Debug info: %d", 123);
    WARN("Warning: something happened");
    ERROR("Error: failed to connect");

    z_log_exit();
    return 0;
}
```

### 输出到文件

```c
#define LOG_FILE_PATH "app.log"
#define Z_LOG_IMPLEMENTATION
#include "z-log.h"

int main() {
    z_log_init();

    INFO("Logging to file");
    // ...

    z_log_exit();
    return 0;
}
```

## 编译

```bash
gcc -o example example.c -lpthread -Wall
```

注意：需要链接 pthread 库。

## 许可证

MIT License
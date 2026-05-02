#ifndef Z_UTIL_H
#define Z_UTIL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#define Z_UTIL_VERSION_MAJOR 1
#define Z_UTIL_VERSION_MINOR 0
#define Z_UTIL_VERSION_PATCH 0
#define Z_UTIL_VERSION_STRING "1.0.0"

#define Z_UTIL_CONCAT_(a, b) a##b
#define Z_UTIL_CONCAT(a, b) Z_UTIL_CONCAT_(a, b)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__) || defined(__clang__)
#define Z_UTIL_METHOD_DEPRECATED __attribute__((deprecated))
#define Z_UTIL_METHOD_UNUSED __attribute__((unused))
#elif defined(_MSC_VER)
#define Z_UTIL_METHOD_DEPRECATED __declspec(deprecated)
#define Z_UTIL_METHOD_UNUSED
#else
#define Z_UTIL_METHOD_DEPRECATED
#define Z_UTIL_METHOD_UNUSED
#endif

#define Z_UTIL_DEFAULT_INDENT 2

#ifndef Z_UTIL_ENABLE_JSON
#define Z_UTIL_ENABLE_JSON 1
#endif

#ifndef Z_UTIL_ENABLE_YAML
#define Z_UTIL_ENABLE_YAML 1
#endif

#ifdef Z_UTIL_IMPLEMENTATION
#if Z_UTIL_ENABLE_JSON
#ifndef Z_JSON_IMPLEMENTATION
#define Z_JSON_IMPLEMENTATION
#endif
#endif
#if Z_UTIL_ENABLE_YAML
#ifndef Z_YAML_IMPLEMENTATION
#define Z_YAML_IMPLEMENTATION
#endif
#endif
#endif

#if Z_UTIL_ENABLE_JSON
#include "z-json/z-json.h"
#endif

#if Z_UTIL_ENABLE_YAML
#include "z-yaml/z-yaml.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* Z_UTIL_H */

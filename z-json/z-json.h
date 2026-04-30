#ifndef Z_JSON_H
#define Z_JSON_H

#ifndef Z_JSON_DEF 
#define Z_JSON_DEF
#endif // Z_JSON_DEF

typedef enum {
    STRING,
    ARRAY,
} JsonType;

struct ZJson;
typedef struct ZJson {
    char* key;
    ZJson** children;
    void* value;
    void** values;
    JsonType type;
} ZJson;

#define Z_JSON_IMPLEMENTATION

#ifndef Z_JSON_IMPLEMENTATION

#endif

#endif //Z_JSON_H

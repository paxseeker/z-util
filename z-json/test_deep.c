#include <stdio.h>
#include <stdlib.h>
#define Z_JSON_IMPLEMENTATION
#include "z-json.h"

int main(void) {
    const char *json = 
        "{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":{\"f\":{\"g\":{\"h\":{\"i\":{\"j\":\"deep_value\"}}}}}}}}}}";
    
    ZJson *root = z_json_parse_str(json);
    if (!root) { printf("Parse failed\n"); return 1; }
    
    // 测试 10 层深度
    ZJson *val = z_json_get_path(root, "a.b.c.d.e.f.g.h.i.j");
    if (val && val->type == Z_JSON_STRING && strcmp(val->value.str_val, "deep_value") == 0) {
        printf("10 层深度访问成功: %s\n", val->value.str_val);
    } else {
        printf("10 层深度访问失败\n");
    }
    
    // 测试 20 层深度
    const char *json20 = "{\"l1\":{\"l2\":{\"l3\":{\"l4\":{\"l5\":{\"l6\":{\"l7\":{\"l8\":{\"l9\":{\"l10\":{\"l11\":{\"l12\":{\"l13\":{\"l14\":{\"l15\":{\"l16\":{\"l17\":{\"l18\":{\"l19\":{\"l20\":\"twenty!\"}}}}}}}}}}}}}}}}}}";
    ZJson *root20 = z_json_parse_str(json20);
    if (root20) {
        ZJson *val20 = z_json_get_path(root20, "l1.l2.l3.l4.l5.l6.l7.l8.l9.l10.l11.l12.l13.l14.l15.l16.l17.l18.l19.l20");
        if (val20 && val20->type == Z_JSON_STRING) {
            printf("20 层深度访问成功: %s\n", val20->value.str_val);
        }
        z_json_free(root20);
    }
    
    z_json_free(root);
    return 0;
}

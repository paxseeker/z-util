#define Z_LOG_IMPLEMENTATION
#include "../z-log.h"

int main() {
    Z_LOG_INIT();

    INFO("hello world");

    Z_LOG_EXIT();
}

// linux_port.cpp
#include "linux_port.hpp"
#include <time.h>
#include <unistd.h>

int64_t platform_now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec)  * 1000LL
         + static_cast<int64_t>(ts.tv_nsec) / 1000000LL;
}

void platform_sleep_ms(uint32_t ms) {
    usleep(ms * 1000u);
}

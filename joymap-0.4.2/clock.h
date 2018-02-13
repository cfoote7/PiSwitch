#include <sys/types.h>
#include <sys/time.h>

static __uint64_t clock_millis() {
    struct timeval tv;
    static __uint64_t last = 0;
    int result = gettimeofday(&tv, NULL);
    if (result != 0) {
        printf("Failed to read clock\n");
        return last;
    }
    last = (__uint64_t)tv.tv_sec * 1000 + (__uint64_t)tv.tv_usec / 1000;
    return last;
}

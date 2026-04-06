

#pragma once
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define TEST(name) printf("Running test: %s\n", #name); name()
#define ASSERT(cond) assert(cond)
#define ASSERT_STR_EQ(a, b) assert(strcmp(a, b) == 0)

#define ASSERT_MSG(cond, fmt, ...) do { \
    if (!(cond)) { \
        fprintf(stderr, "Assertion failed: " fmt "\n", ##__VA_ARGS__); \
        fprintf(stderr, "    at %s:%d\n", __FILE__, __LINE__); \
        exit(1); \
    } \
} while(0)

#ifdef __GNUC__
__attribute__((constructor))
static void set_timeout() {
    alarm(10);
}
#endif


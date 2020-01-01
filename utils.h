#ifndef UTILS1_H
#define UTILS1_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define to_container(type, ptr, member) (type*)((void*)((char*)ptr - offsetof(type, member)))

#define ARRAY_SIZE(x)   ((sizeof(x)) / (sizeof(x[0])))

#define IS_BIT_SET(flag, bit) ((flag) & (1<<bit))

#define SET_BIT(flag, bit) (flag |= (1<<(bit)))

#define CLEAR_BIT(flag, bit) (flag &= ~(1<<bit))

#define CASE2STR(x) case x: return #x

#define ASSERT(x) \
    if (!(x)) { \
        fprintf(stderr, "ASSERT: %s:%d failed\n", __FILE__, __LINE__); \
        abort(); \
    }

#define UNUSED(x) ((void)(x))

#define MAX(a, b) ((a >= b) ? a : b)
#define MIN(a, b) ((a <= b) ? a : b)

#define PTHREAD_CHK(expr) do { if (expr != 0) {assert(0);fprintf(stderr, "pthread call error\n");};} while(0)
#define SYSCALL_CHK(expr) do { if (expr != 0) {assert(0);fprintf(stderr, "syscall error\n");};} while(0)

#ifdef __cplusplus
}
#endif

#endif


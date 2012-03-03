
#ifndef _EXCEPT_H_
#define _EXCEPT_H_

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>


/* Error handling magic */
typedef int ExceptionType;

struct _exception {
    const char *    file;
    int             line;
    const char *    function;
    ExceptionType   type;
    const char *    msg;
};
typedef struct _exception Exception;

static jmp_buf _exception_env;
static Exception _exception_list[16] = { { 0 } };
static int _exception_stack = 0;

#define _try \
    if (!setjmp(_exception_env))

#define _catch(_e) \
    Exception *_e = NULL; \
    if (_exception_stack) { \
        _e = &_exception_list[_exception_stack--]; \
    } \
    if (_e)

#define _throw(_e) \
    if (_exception_stack >= (sizeof(_exception_list) / sizeof(Exception)) - 1) { \
        fprintf(stderr, "FATAL: TOO MANY NESTED EXCEPTIONS\n"); \
        abort(); \
    } \
    memcpy(&_exception_list[++_exception_stack], &_e, sizeof(Exception)); \
    longjmp(_exception_env, _exception_stack);

#define EXCEPTION_INFO __FILE__, __LINE__, __FUNCTION__

#endif /* _EXCEPT_H_ */

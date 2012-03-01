
#ifndef _EXCEPT_H_
#define _EXCEPT_H_

#include <setjmp.h>


/* Error handling magic */
static jmp_buf _exception_env;

typedef int ExceptionType;

struct _exception {
    const char *    file;
    int             line;
    const char *    function;
    ExceptionType   type;
    const char *    msg;
};
typedef struct _exception Exception;

#define _try(_e) \
    Exception *_e = (Exception *)setjmp(_exception_env); \
    if (!_e)

#define _catch \
    else

#define _throw(_e) \
    longjmp(_exception_env, (int)&_e)

#define EXCEPTION_INFO __FILE__, __LINE__, __FUNCTION__

#endif /* _EXCEPT_H_ */

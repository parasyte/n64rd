
#include <setjmp.h>

#include "except.h"


/* Protected variables */
jmp_buf _exception_env = { { { 0 } } };
Exception _exception_list[_EXCEPTION_LIST_SIZE] = { { 0 } };
int _exception_stack = 0;

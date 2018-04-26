#ifndef _PK_CONSOLE_H
#define _PK_CONSOLE_H

#include "pk.h"


void vprintk(const char* s, va_list vl);
void printk(const char* s, ...);
void dump_tf(trapframe_t* tf);
void do_panic(const char* s, ...);
void kassert_fail(const char* s);

#endif

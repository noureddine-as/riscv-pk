#ifndef _SBI_CALLS_H
#define _SBI_CALLS_H

#include "encoding.h"
#include <stdint.h>

uintptr_t sbi_call_putchar(char ch);
uintptr_t sbi_call_set_timer();
uintptr_t sbi_call_set_timer_MAX();


#endif
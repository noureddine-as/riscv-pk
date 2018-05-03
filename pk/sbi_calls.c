#include "sbi_calls.h"

// Get SBI_ codes
#include "mcall.h"


uintptr_t sbi_call_putchar(char ch) //uintptr_t arg0, uintptr_t code)
{
  register uintptr_t a0 asm ("a0") = (uintptr_t)ch;
  register uintptr_t a1 asm ("a1"); // = 'P';
  register uintptr_t a7 asm ("a7") = SBI_CONSOLE_PUTCHAR;
  asm volatile ("ecall" : "=r" (a0) : "r" (a0), "r" (a1), "r" (a7));

  return a0;
}

uintptr_t sbi_call_set_timer(uint64_t next_time) //uintptr_t arg0, uintptr_t code)
{
  register uintptr_t a0 asm ("a0") = (uintptr_t)next_time;
  register uintptr_t a1 asm ("a1"); // = 'P';
  register uintptr_t a7 asm ("a7") = SBI_SET_TIMER;
  asm volatile ("ecall" : "=r" (a0) : "r" (a0), "r" (a1), "r" (a7));

  return a0;
}

uintptr_t sbi_call_set_timer_MAX() //uintptr_t arg0, uintptr_t code)
{
  register uintptr_t a0 asm ("a0") = (uintptr_t)(-1ULL);
  register uintptr_t a1 asm ("a1"); // = 'P';
  register uintptr_t a7 asm ("a7") = SBI_SET_TIMER;
  asm volatile ("ecall" : "=r" (a0) : "r" (a0), "r" (a1), "r" (a7));

  return a0;
}




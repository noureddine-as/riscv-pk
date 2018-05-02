#include "pk.h"
#include "frontend.h"
#include "atomic.h"
#include "mtrap.h"
#include "mcall.h"
#include "encoding.h"

volatile uint8_t final_ret = 0;

#define	RET_LIMIT 		0x0F


void foo(int cid)
{	
	printk("Hello, from foo() -- Core %d \n", cid);
	final_ret += (1 << cid);
}


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


extern volatile uint64_t* mtime;
void supervisor_main(long cid, char** argv)
{
	printk("[Core %d] Time now is %ul \n", cid, *mtime);
	printk("[Core %d] Hello world  1, from main\n", cid);

    while(cid != 0){
	    wfi();
	}

	if(cid == 0){
		uint64_t next_timer_int = *mtime + 1000;
		printk("[Core %d] Will configure timer on %d \n", cid, next_timer_int);
		sbi_call_set_timer(next_timer_int);
	}

	//int ret = (int)(sbi_call_putchar('\n'));
	//printk("Return from SBI call = %d \n", ret);

	volatile int a = 500;
	while(a--){
		if(a % 10 == 0) printk("Instant %d \n", *mtime);
	    wfi();};

	printk("[Core %d] Hello world  2, from main\n", cid);

	shutdown(99);
}


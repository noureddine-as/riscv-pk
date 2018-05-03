#include "pk.h"
#include "frontend.h"
//#include "atomic.h"
#include "mtrap.h"
//#include "encoding.h"
#include "sbi_calls.h"

extern volatile uint64_t* mtime;

volatile uint8_t final_ret = 0;
#define	RET_LIMIT 		0x0F
void foo(int cid)
{	
	printk("Hello, from foo() -- Core %d \n", cid);
	final_ret += (1 << cid);
}

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


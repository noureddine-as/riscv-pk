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

/*


void sbi_call_shutdown(void){

    asm volatile ("li a7, %0 \n\t" 
          		  "ecall\n\t"
		         :       // output 
         		 :"I" (SBI_SHUTDOWN)         // input 
         		 :        // clobbered register 
         		 );

}



void sbi_call_ipi(void){

    asm volatile ("li a0, 14 \n\t" // Twichiat li baghi tfiyye9, Core 1 (<< 1) 
    			  "li a7, 4 \n\t" //%0 \n\t" 
          		  "ecall\n\t"
		         :       // output 
         		 : //"I" (SBI_SEND_IPI)         // input 
         		 : //"a0", "a7"       // clobbered register 
         		 );

}

void sbi_clean_ipi(void){

    asm volatile (//"li a0, 2 \n\t" // Twichiat li baghi tfiyye9, Core 1 (<< 1) 
    			  "li a7, %0 \n\t" 
          		  "ecall\n\t"
		         :       // output 
         		 :"I" (SBI_CLEAR_IPI)         // input 
         		 :        // clobbered register 
         		 );

}


void sbi_call_set_timer_relative(int offset){
	uint64_t next_interruption = *mtime + offset;

    asm volatile ("ld t0, %1\n\t" 
    			  "add a0, x0, t0\n\t"
    			  "li a7, %0\n\t"
          		  "ecall\n\t"
		         :       // output 
         		 :"I" (SBI_SET_TIMER), "m" (next_interruption)         // input 
         		 : // "a0", "a1", "a2", "a3", "a4", "a7"        // clobbered register 
         		 );

}

void sbi_call_set_timer(uint64_t instant){

    asm volatile ("ld t0, %1\n\t" 
    			  "add a0, x0, t0\n\t"
    			  "li a7, %0\n\t"
          		  "ecall\n\t"
		         : //"=r" (t0), "=r" (a0), "=r" (a7)       // output 
         		 :"I" (SBI_SET_TIMER), "r" (instant)         // input 
         		 : "t0", "a0", "a7" // "a0", "a1", "a2", "a3", "a4", "a7"        // clobbered register 
         		 );

}


const uint64_t TIMER_CMP_VAL = 16000;
void sbi_call_set_timer_absolute_const(){

    asm volatile ("la t0, TIMER_CMP_VAL\n\t"
    			  "ld t1, (t0)\n\t"
    			  "add a0, x0, t1\n\t"
    			  "li a7, %0\n\t"
          		  "ecall\n\t"
		         :       // output 
         		 :"I" (SBI_SET_TIMER)     // input 
         		 : "t0", "t1", "a0", "a7"             // clobbered register 
         		 );

}
*/


uintptr_t sbi_call_putchar(char ch) //uintptr_t arg0, uintptr_t code)
{
  register uintptr_t a0 asm ("a0") = (uintptr_t)ch;
  register uintptr_t a1 asm ("a1"); // = 'P';
  register uintptr_t a7 asm ("a7") = SBI_CONSOLE_PUTCHAR;
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
		//sbi_call_set_timer_absolute_const();
		//sbi_call_set_timer(next_timer_int);
	}

	//meth();meth();meth();meth();

	int ret = (int)(sbi_call_putchar('\n'));
	printk("Return from SBI call = %d \n", ret);

	//volatile int a = 500;
	//while(a--){
	//	printk("Instant %d \n", *mtime);
	//    wfi();};

	printk("[Core %d] Hello world  2, from main\n", cid);

	shutdown(99);
}


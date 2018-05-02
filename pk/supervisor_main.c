#include "pk.h"
#include "frontend.h"
#include "atomic.h"
#include "mtrap.h"
#include "mcall.h"

volatile uint8_t final_ret = 0;

#define	RET_LIMIT 		0x0F

static spinlock_t print_lock = SPINLOCK_INIT;

void wake_hart(int hart)
{
	if (hart == 0)
		*(HLS()->ipi) = 1;
	else
      *(OTHER_HLS(hart)->ipi) = 1; // wakeup the hart
}

void foo(int cid)
{	
	printk("Hello, from foo() -- Core %d \n", cid);
	final_ret += (1 << cid);
}

void machine_main(long cid, char** argv){

}


//static uintptr_t mcall_clear_ipi()
//extern void send_ipi(uintptr_t recipient, int event);
//static void send_ipi_many(uintptr_t* pmask, int event)

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

void sbi_call_set_timer_absolute(int instant){

    asm volatile ("ld t0, %1\n\t" 
    			  "add a0, x0, t0\n\t"
    			  "li a7, %0\n\t"
          		  "ecall\n\t"
		         :       // output 
         		 :"I" (SBI_SET_TIMER), "m" (instant)         // input 
         		 : // "a0", "a1", "a2", "a3", "a4", "a7"        // clobbered register 
         		 );

}

//#define TIMER_ABS_CALL	15000

const uint64_t TIMER_CMP_VAL = 16000;
void sbi_call_set_timer_absolute_const(){

    asm volatile ("la t0, TIMER_CMP_VAL\n\t"
    			  "ld t1, (t0)\n\t"
    			  "add a0, x0, t1\n\t"
    			  "li a7, %0\n\t"
          		  "ecall\n\t"
		         :       // output 
         		 :"I" (SBI_SET_TIMER) //, "I" (TIMER_ABS_CALL)         // input 
         		 : "t0", "t1" // "a0", "a1", "a2", "a3", "a4", "a7"        // clobbered register 
         		 );

}
/*

extern void handle_trap(trapframe_t* tf);
void init_interrupts()
{

  uintptr_t sstatus = read_csr(sstatus);
  sstatus = INSERT_FIELD(sstatus, MSTATUS_MPP, PRV_S);
  sstatus = INSERT_FIELD(sstatus, MSTATUS_MPIE, 1);
 // mstatus = INSERT_FIELD(mstatus, MSTATUS_MPIE, 0);  // Original
  write_csr(sstatus, sstatus);
  //write_csr(mscratch, MACHINE_STACK_TOP() - MENTRY_FRAME_SIZE);
  //write_csr(sepc, handle_trap);
}*/

extern volatile uint64_t* mtime;
void supervisor_main(long cid, char** argv)
{




	//poweroff(99);
	printk("Time now is %ud \n", *mtime);

	printk("Hello world  1  , from main, Core: %d \n", cid);

    while(cid != 0){
	    wfi();};

	if(cid == 0){
		//printk("will send IPI now, Core: %d \n", cid);
		printk("Configuring timer, Core: %d,     mtime=%d   next_trigger=%d \n", cid, *mtime, *mtime + 2000);
		
		sbi_call_set_timer_absolute_const();
		//sbi_call_ipi();
		//sbi_call_set_timer_relative(2000);
		//sbi_call_ipi(); // ---> provokes unhandlable trap at 80003634
		//sbi_clean_ipi();
		//sbi_call_shutdown();
		//uintptr_t sbi_call_code = SBI_SHUTDOWN;
    

		//send_ipi(1, IPI_SOFT );
		// asm volatile ("li a7, %0\n\t"
         //       "ecall\n\t"
        //        : : "r" (SBI_SHUTDOWN) );

                //"csrw pmpaddr0, %1\n\t"
                //"csrw pmpcfg0, %0\n\t"
                //".align 2\n\t"
                //"1: csrw mtvec, t0"
                //: : "r" (pmpc), "r" (-1UL) : "t0");

		//uintptr_t time = read_csr(CSR_TIME);

		//printk("Time now is %ud \n", *mtime);

	}

	volatile int a = 500;
	while(a--){
		printk("Instant %d \n", *mtime);
	    wfi();};

	//while(1){

		//printk("wait ... \n");
	//}

//sbi_clean_ipi();

	/*
  la s0, str
1:
  lbu a0, (s0)
  beqz a0, 1f
  li a7, SBI_CONSOLE_PUTCHAR
  ecall
  add s0, s0, 1
  j 1b

1:
  li a7, SBI_SHUTDOWN
  ecall



	*/

//sbi_call_shutdown();
	printk("Hello world  2  , from main, Core: %d \n", cid);

	putstring("using putstring ...\n");

/*
	// Core1: waits here until waken up by core 0
	if(cid == 1 && ((OTHER_HLS(1)->mipi_pending) == 0))
		__asm__ volatile("wfi");

	// Core0: executes foo(0)
	//    	  waits here until waken up by core 1
	if(cid == 0){
		foo(cid);
		wake_hart(1);
		while(HLS()->mipi_pending == 0)
			__asm__ volatile("wfi");
	}

	// Core1: executes foo(1)
	//    	  waits here forever
	if(cid == 1){
		foo(cid);
		wake_hart(0);
		while((OTHER_HLS(1)->mipi_pending) == 0)
			__asm__ volatile("wfi");
	}


	// here core 0 comes
	//while(final_ret < RET_LIMIT || cid > 0);
	while(final_ret < 3);
	printk("---------- Finishing ... Core %d --------- \n", cid);
*/
	shutdown(99);
}


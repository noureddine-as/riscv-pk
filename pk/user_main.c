#include "pk.h"
#include "frontend.h"

#include <stdio.h>

void user_main(void)
{
	printm("Hello world, from REAAAL user main !!!!! .... printm \n");
	printk("Hello world, from REAAAL user main !!!!! .... printk \n");
	//printf("Hello world, from user main !!!!! .... printf \n");

	// If we put return we enter to the trap handler with an undefined args
	// Hence we get
	// assertion failed @ 0x000000008000172a: tf->cause < ARRAY_SIZE(trap_handlers) && trap_handlers[tf->cause]
	// return;
	shutdown(99);
	panic("Ending program !");
	//return 99;
}
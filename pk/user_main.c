#include "pk.h"
#include "frontend.h"

#include <stdio.h>

void user_main(void)
{
	printk("Hello world, from main, S-Mode.\n");

	shutdown(99);
}
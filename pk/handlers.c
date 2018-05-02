// See LICENSE for license details.

#include "pk.h"
#include "config.h"
#include "syscall.h"
#include "mmap.h"

static void handle_illegal_instruction(trapframe_t* tf)
{
  tf->insn = *(uint16_t*)tf->epc;
  int len = insn_len(tf->insn);
  if (len == 4)
    tf->insn |= ((uint32_t)*(uint16_t*)(tf->epc + 2) << 16);
  else
    kassert(len == 2);

  dump_tf(tf);
  panic("An illegal instruction was executed!");
}

static void handle_breakpoint(trapframe_t* tf)
{
  dump_tf(tf);
  printk("Breakpoint!\n");
  tf->epc += 4;
}

static void handle_misaligned_fetch(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned instruction access!");
}

static void handle_fault_load_access(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Load access fault!");
}

static void handle_misaligned_store(trapframe_t* tf)
{
  dump_tf(tf);
  panic("Misaligned AMO!");
}

static void segfault(trapframe_t* tf, uintptr_t addr, const char* type)
{
  dump_tf(tf);
  const char* who = (tf->status & SSTATUS_SPP) ? "Kernel" : "User";
  panic("%s %s segfault @ %p", who, type, addr);
}

static void handle_fault_fetch(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_EXEC) != 0)
    segfault(tf, tf->badvaddr, "fetch");
}

static void handle_fault_load(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_READ) != 0)
    segfault(tf, tf->badvaddr, "load");
}

static void handle_fault_store(trapframe_t* tf)
{
  if (handle_page_fault(tf->badvaddr, PROT_WRITE) != 0)
    segfault(tf, tf->badvaddr, "store");
}

static void handle_syscall(trapframe_t* tf)
{
  tf->gpr[10] = do_syscall(tf->gpr[10], tf->gpr[11], tf->gpr[12], tf->gpr[13],
                           tf->gpr[14], tf->gpr[15], tf->gpr[17]);
  tf->epc += 4;
}

uintptr_t sbi_call_set_timer_MAX() //uintptr_t arg0, uintptr_t code)
{
  register uintptr_t a0 asm ("a0") = (uintptr_t)(-1ULL);
  register uintptr_t a1 asm ("a1"); // = 'P';
  register uintptr_t a7 asm ("a7") = 0;
  asm volatile ("ecall" : "=r" (a0) : "r" (a0), "r" (a1), "r" (a7));

  return a0;
}

uintptr_t sbi_call_set_timer_step() //uintptr_t arg0, uintptr_t code)
{
  register uintptr_t a0 asm ("a0") = (uintptr_t)(*mtime + 1000);
  register uintptr_t a1 asm ("a1"); // = 'P';
  register uintptr_t a7 asm ("a7") = 0;
  asm volatile ("ecall" : "=r" (a0) : "r" (a0), "r" (a1), "r" (a7));

  return a0;
}

static void handle_interrupt(trapframe_t* tf)
{

  sbi_call_set_timer_MAX();
  //clear_csr(sip, SIP_STIP);
  write_csr(sip, 0);
  printk("[ General Interruption Handler ] .. Clearing SIP and SIE ... t=%d\n", *mtime);
  
}

static void handle_supervisor_timer_interrupt(trapframe_t* tf)
{
  // https://github.com/coreboot/coreboot/blob/master/src/arch/riscv/trap_handler.c
  // The only way to reset the timer interrupt is to
  // write mtimecmp. But we also have to ensure the
  // comparison fails, for a long time, to let
  // supervisor interrupt handler compute a new value
  // and set it. Finally, it fires if mtimecmp is <=
  // mtime, not =, so setting mtimecmp to 0 won't work
  // to clear the interrupt and disable a new one. We
  // have to set the mtimecmp far into the future.
  // Awkward!

  printk("[ CAUSE_SUPERVISOR_TIMER_INTERRUPT ] .. Setting timecmp to max and Clearing STIP ... t=%d\n", *mtime);
  sbi_call_set_timer_MAX();
  clear_csr(sip, SIP_STIP);
}

void handle_trap(trapframe_t* tf)
{
  printk("[ handle_trap ]     cause=%llx\n", (intptr_t)tf->cause);

  //if ( (tf->cause) & (1ULL << (__riscv_xlen - 1) ) ){ //(intptr_t)tf->cause < 0){
  if((intptr_t)tf->cause < 0){
    uint64_t int_code = tf->cause & ~0x8000000000000000ULL;


    typedef void (*interrupt_handler)(trapframe_t*);
    const static interrupt_handler interrupt_handlers[] = {
      // [CAUSE_USER_SOFTWARE_INTERRUPT] = handle_supervisor_timer_interrupt,
      // [CAUSE_SUPERVISOR_SOFTWARE_INTERRUPT] = handle_supervisor_timer_interrupt,
      // [CAUSE_MACHINE_SOFTWARE_INTERRUPT] = handle_supervisor_timer_interrupt,

      //[CAUSE_USER_TIMER_INTERRUPT] = handle_supervisor_timer_interrupt,
      [CAUSE_SUPERVISOR_TIMER_INTERRUPT] = handle_supervisor_timer_interrupt
      //[CAUSE_MACHINE_TIMER_INTERRUPT] = handle_supervisor_timer_interrupt,

      //[CAUSE_USER_EXTERNAL_INTERRUPT] = handle_supervisor_timer_interrupt,
      //[CAUSE_SUPERVISOR_EXTERNAL_INTERRUPT] = handle_supervisor_timer_interrupt,
      //[CAUSE_MACHINE_EXTERNAL_INTERRUPT] = handle_supervisor_timer_interrupt,

    };

    //if(tf->cause < ARRAY_SIZE(trap_handlers) && trap_handlers[tf->cause]);
    if( int_code < ARRAY_SIZE(interrupt_handlers) && interrupt_handlers[int_code])
      return interrupt_handlers[int_code](tf);
    else
      return handle_interrupt(tf);
  }

  typedef void (*trap_handler)(trapframe_t*);

  const static trap_handler trap_handlers[] = {
    [CAUSE_MISALIGNED_FETCH] = handle_misaligned_fetch,
    [CAUSE_FETCH_PAGE_FAULT] = handle_fault_fetch,
    [CAUSE_ILLEGAL_INSTRUCTION] = handle_illegal_instruction,

    // Added by me
    [CAUSE_LOAD_ACCESS] = handle_fault_load_access,

    [CAUSE_USER_ECALL] = handle_syscall,
    [CAUSE_BREAKPOINT] = handle_breakpoint,
    [CAUSE_MISALIGNED_STORE] = handle_misaligned_store,
    [CAUSE_LOAD_PAGE_FAULT] = handle_fault_load,
    [CAUSE_STORE_PAGE_FAULT] = handle_fault_store
  };

  kassert(tf->cause < ARRAY_SIZE(trap_handlers) && trap_handlers[tf->cause]);

  trap_handlers[tf->cause](tf);
}

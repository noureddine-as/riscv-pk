#include "pk.h"
#include "mmap.h"
#include "boot.h"
#include "elf.h"
#include "mtrap.h"
#include "frontend.h"

/*******************/

/*   to include atomic related functions, like mb()   */
#include "atomic.h"

/*   to include filters and device tree kernel printers    */
#include "fdt_k.h"

/*******************************/


#include <stdbool.h>

static const void* entry_point;

elf_info current;
long disabled_hart_mask = 0;

static void handle_option(const char* s)
{
  switch (s[1])
  {
    case 's': // print cycle count upon termination
      current.cycle0 = 1;
      break;

    case 'p': // disable demand paging
      demand_paging = 0;
      break;

    default:
      panic("unrecognized option: `%c'", s[1]);
      break;
  }
}

#define MAX_ARGS 64
typedef union {
  uint64_t buf[MAX_ARGS];
  char* argv[MAX_ARGS];
} arg_buf;

static size_t parse_args(arg_buf* args)
{
  long r = frontend_syscall(SYS_getmainvars, va2pa(args), sizeof(*args), 0, 0, 0, 0, 0);
  kassert(r == 0);
  uint64_t* pk_argv = &args->buf[1];
  // pk_argv[0] is the proxy kernel itself.  skip it and any flags.
  size_t pk_argc = args->buf[0], arg = 1;
  for ( ; arg < pk_argc && *(char*)(uintptr_t)pk_argv[arg] == '-'; arg++)
    handle_option((const char*)(uintptr_t)pk_argv[arg]);

  for (size_t i = 0; arg + i < pk_argc; i++)
    args->argv[i] = (char*)(uintptr_t)pk_argv[arg + i];
  return pk_argc - arg;
}

static void init_tf(trapframe_t* tf, long pc, long sp)
{
  memset(tf, 0, sizeof(*tf));
  tf->status = (read_csr(sstatus) &~ SSTATUS_SPP &~ SSTATUS_SIE) | SSTATUS_SPIE;
  tf->gpr[2] = sp;
  tf->epc = pc;
}

static void run_loaded_program(size_t argc, char** argv, uintptr_t kstack_top)
{
  // copy phdrs to user stack
  size_t stack_top = current.stack_top - current.phdr_size;
  memcpy((void*)stack_top, (void*)current.phdr, current.phdr_size);
  current.phdr = stack_top;

  // copy argv to user stack
  for (size_t i = 0; i < argc; i++) {
    size_t len = strlen((char*)(uintptr_t)argv[i])+1;
    stack_top -= len;
    memcpy((void*)stack_top, (void*)(uintptr_t)argv[i], len);
    argv[i] = (void*)stack_top;
  }

  // copy envp to user stack
  const char* envp[] = {
    // environment goes here
  };
  size_t envc = sizeof(envp) / sizeof(envp[0]);
  for (size_t i = 0; i < envc; i++) {
    size_t len = strlen(envp[i]) + 1;
    stack_top -= len;
    memcpy((void*)stack_top, envp[i], len);
    envp[i] = (void*)stack_top;
  }

  // align stack
  stack_top &= -sizeof(void*);

  struct {
    long key;
    long value;
  } aux[] = {
    {AT_ENTRY, current.entry},
    {AT_PHNUM, current.phnum},
    {AT_PHENT, current.phent},
    {AT_PHDR, current.phdr},
    {AT_PAGESZ, RISCV_PGSIZE},
    {AT_SECURE, 0},
    {AT_RANDOM, stack_top},
    {AT_NULL, 0}
  };

  // place argc, argv, envp, auxp on stack
  #define PUSH_ARG(type, value) do { \
    *((type*)sp) = (type)value; \
    sp += sizeof(type); \
  } while (0)

  #define STACK_INIT(type) do { \
    unsigned naux = sizeof(aux)/sizeof(aux[0]); \
    stack_top -= (1 + argc + 1 + envc + 1 + 2*naux) * sizeof(type); \
    stack_top &= -16; \
    long sp = stack_top; \
    PUSH_ARG(type, argc); \
    for (unsigned i = 0; i < argc; i++) \
      PUSH_ARG(type, argv[i]); \
    PUSH_ARG(type, 0); /* argv[argc] = NULL */ \
    for (unsigned i = 0; i < envc; i++) \
      PUSH_ARG(type, envp[i]); \
    PUSH_ARG(type, 0); /* envp[envc] = NULL */ \
    for (unsigned i = 0; i < naux; i++) { \
      PUSH_ARG(type, aux[i].key); \
      PUSH_ARG(type, aux[i].value); \
    } \
  } while (0)

  STACK_INIT(uintptr_t);

  if (current.cycle0) { // start timer if so requested
    current.time0 = rdtime();
    current.cycle0 = rdcycle();
    current.instret0 = rdinstret();
  }

  trapframe_t tf;
  init_tf(&tf, current.entry, stack_top);
  __clear_cache(0, 0);
  write_csr(sscratch, kstack_top);
  start_user(&tf);
}

/*
static void rest_of_boot_loader(uintptr_t kstack_top)
{

  printm("Hello world, from user main !!!!! .... printm \n");
  printk("Hello world, from user main !!!!! .... printk \n");
  //printf("Hello world, from user main !!!!! .... printf \n");


  arg_buf args;
  size_t argc = parse_args(&args);
  if (!argc)
    panic("tell me what ELF to load!");

  // load program named by argv[0]
  long phdrs[128];
  current.phdr = (uintptr_t)phdrs;
  current.phdr_size = sizeof(phdrs);
  load_elf(args.argv[0], &current);

  run_loaded_program(argc, args.argv, kstack_top);
}
void boot_loader(uintptr_t dtb)
{
  printm("Beginning of bootloader !!!!! .... printm \n");
  printk("Beginning of bootloader !!!!! .... printk \n");

  extern char trap_entry;
  write_csr(stvec, &trap_entry);
  write_csr(sscratch, 0);
  write_csr(sie, 0);
  set_csr(sstatus, SSTATUS_SUM);

  printm("Before file_init !!!!! .... printm \n");
  printk("Before file_init !!!!! .... printk \n");

  file_init();

  printm("After file_init !!!!! .... printm \n");
  printk("After file_init !!!!! .... printk \n");

  enter_supervisor_mode(rest_of_boot_loader, pk_vm_init(), 0);
}

void boot_other_hart(uintptr_t dtb)
{
  // stall all harts besides hart 0
  while (1)
    wfi();
}
*/


static uintptr_t dtb_output()
{
  // extern char _payload_end;
  extern char _end;
  
  //uintptr_t end = (uintptr_t) &_payload_end;
  uintptr_t end = (uintptr_t) &_end;

  return (end + MEGAPAGE_SIZE - 1) / MEGAPAGE_SIZE * MEGAPAGE_SIZE;
}

static void filter_dtb(uintptr_t source)
{
  uintptr_t dest = dtb_output();
  uint32_t size = fdt_size(source);
  memcpy((void*)dest, (void*)source, size);

  // Remove information from the chained FDT
  filter_harts(dest, &disabled_hart_mask);
  filter_plic(dest);
  filter_compat(dest, "riscv,clint0");
  filter_compat(dest, "riscv,debug-013");
}

void boot_loader(uintptr_t dtb)
{
  // ONLY M works here !
  printm("Launching bootloader ...\n");
  printk("Launching bootloader ...\n");

  extern char trap_entry;
  write_csr(stvec, &trap_entry);
  write_csr(sscratch, 0);
  //write_csr(sie, 0);   // ORIGINAL
  
  //            Enabling Software IE and Timer IE
  write_csr(sie,  1 << 5 | 1 << 4 | 1 << 1 | 1); // let's define interrupts that are enabled in Supervisor mode.


  // set_csr(sstatus, SSTATUS_SUM);  // ORIGINAL
    set_csr(sstatus, SSTATUS_SUM | SSTATUS_SIE | SSTATUS_UIE);


  //  We need this so that printk works ! 
  file_init();

  extern void* supervisor_main;
  filter_dtb(dtb);
#ifdef PK_ENABLE_LOGO
  printm("................................................\n"
         "|                TIMA LABORATORY               | \n"
         "|          Grenoble INP - UGA - CNRS           | \n"
         "|..............................................|\n");
  printk("................................................\n"
         "|                TIMA LABORATORY               | \n"
         "|          Grenoble INP - UGA - CNRS           | \n"
         "|..............................................|\n");
  //print_logo();
#endif
#ifdef PK_PRINT_DEVICE_TREE
  fdt_printk(dtb_output());
#endif
  mb();
  entry_point = &supervisor_main;
  boot_other_hart(0);
}

void boot_other_hart(uintptr_t unused __attribute__((unused)))
{
  const void* entry;
  do {
    entry = entry_point;
    mb();
  } while (!entry);

  long hartid = read_csr(mhartid);
  if ((1 << hartid) & disabled_hart_mask) {
    while (1) {
      __asm__ volatile("wfi");
#ifdef __riscv_div
      __asm__ volatile("div x0, x0, x0");
#endif
    }
  }


  //extern void machine_main(long cid, char** argv);
  //machine_main(hartid, 0);


  enter_supervisor_mode(entry, hartid, dtb_output());
}


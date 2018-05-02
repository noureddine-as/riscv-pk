pk_subproject_deps = \
	util \
	softfloat \
	machine \

pk_hdrs = \
	boot.h \
	elf.h \
	file.h \
	frontend.h \
	mmap.h \
	pk.h \
	syscall.h \
	fdt_k.h \

pk_c_srcs = \
	file.c \
	syscall.c \
	handlers.c \
	frontend.c \
	elf.c \
	console.c \
	mmap.c \
	fdt_k.c \
	supervisor_main.c \
	
pk_asm_srcs = \
	entry.S \

pk_test_srcs =

pk_install_prog_srcs = \
	pk.c \

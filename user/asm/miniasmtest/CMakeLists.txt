add_executable(minites.exe)

target_sources(minites.exe PRIVATE ../../../kernel/arch/i386/boot/crti.S ../../../kernel/arch/i386/boot/crtn.S asm.S)
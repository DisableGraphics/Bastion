add_executable(testc.exe)

target_sources(testc.exe PRIVATE ../../../kernel/arch/i386/boot/crti.S ../../../kernel/arch/i386/boot/crtn.S main.c)
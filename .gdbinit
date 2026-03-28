file zig-out/bin/kernel.elf
target remote localhost:1234
add-symbol-file core/mmanager/bitmapmm/zig-out/bin/bitmapmm 0x800000

.globl reloadSegments
reloadSegments:
   # Reload CS register containing code selector:
   jmp $0x08,$.reload_CS # 0x08 is a stand-in for your code segment

.reload_CS:
   # Reload data segment registers:
   mov   $0x10,%ax # 0x10 is a stand-in for your data segment
   mov   %ax,%dx
   mov   %ax, %es
   mov   %ax,%fs
   mov   %ax,%gs
   mov   %ax,%ss
   ret

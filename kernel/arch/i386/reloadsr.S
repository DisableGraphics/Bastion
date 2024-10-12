.globl reloadSegments
reloadSegments:
   # Reload CS register containing code selector:
   ljmp $0x08, $.reload_CS # 0x08 is a stand-in for your code segment

.reload_CS:
   # Reload data segment registers:
   movw   0x10,%ax # 0x10 is a stand-in for your data segment
   movw   %ax,%dx
   movw   %ax, %es
   movw   %ax,%fs
   movw   %ax,%gs
   movw   %ax,%ss
   ret
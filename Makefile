BOOT_OBJ=boot/boot.o
KERNEL_SOURCES=$(wildcard kernel/*.cpp)
KERNEL_OBJS=$(KERNEL_SOURCES:.cpp=.o)
OS_NAME=nexa
OS_BIN=$(OS_NAME).bin
OS_ISO=img/$(OS_NAME).iso

all: $(OS_BIN)

$(OS_BIN): $(BOOT_OBJ) $(KERNEL_OBJS)
	i686-elf-g++ -T link/linker.ld -o $(OS_BIN) -ffreestanding -O2 -nostdlib $(BOOT_OBJ) $(KERNEL_OBJS) -lgcc -Lkernel/kclib -lkclib

$(BOOT_OBJ):
	cd boot && make

$(KERNEL_OBJS): $(KERNEL_SOURCES)
	cd kernel && make

iso: $(OS_ISO)

$(OS_ISO): $(OS_BIN)
	cd img && ./create-img

bootiso: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)

clean:
	cd boot && make clean
	cd kernel && make clean
	rm -f $(OS_BIN)
	rm -rf img/isodir img/$(OS_NAME).iso

cross-compiler:
	cd cross-comp && ./create-cross-compiler.sh
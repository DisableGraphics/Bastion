BOOT_OBJ=boot/boot.o
KERNEL_SOURCES=$(wildcard kernel/*.cpp)
KERNEL_OBJS=$(KERNEL_SOURCES:.cpp=.o)
OS_BIN=nexa.bin

all: $(BOOT_OBJ) $(KERNEL_OBJS)
	i686-elf-g++ -T link/linker.ld -o $(OS_BIN) -ffreestanding -O2 -nostdlib $(BOOT_OBJ) $(KERNEL_OBJS) -lgcc -Lkernel/kclib -lkclib

$(BOOT_OBJ):
	cd boot && make

$(KERNEL_OBJS): $(KERNEL_SOURCES)
	cd kernel && make

clean:
	cd boot && make clean
	cd kernel && make clean
	rm -f $(OS_BIN)
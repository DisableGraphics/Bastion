all:
	./build.sh

clean:
	./clean.sh

qemu: all
	./qemu.sh

qemu-debug: all
	./qemu.sh debug
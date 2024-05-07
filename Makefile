all:
	./build.sh

clean:
	./clean.sh

qemu: all
	./qemu.sh
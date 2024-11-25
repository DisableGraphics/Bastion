all:
	./build.sh

clean:
	./clean.sh

qemu: all
	./qemuorig.sh

qemu-debug: all
	./qemuorig.sh debug
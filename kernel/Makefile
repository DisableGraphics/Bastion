KERNEL_SOURCES=$(wildcard *.cpp)
KERNEL_OBJECTS=$(KERNEL_SOURCES:.cpp=.o)
KCLIB=kclib/libkclib.a

all: $(KCLIB) $(KERNEL_OBJECTS)
	i686-elf-ld -r -o kernel.o $(KERNEL_OBJECTS)

%.o: %.cpp
	i686-elf-g++ -c $^ -o $@ -ffreestanding -O2 -Wall -Wextra -fno-exceptions -fno-rtti -Lkclib -lkclib -Ikclib/include

$(KCLIB):
	cd kclib && make

clean:
	rm *.o
	cd kclib && make clean
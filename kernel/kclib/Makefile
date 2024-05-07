SOURCES=$(wildcard src/*.c)
OBJECTS=$(SOURCES:.c=.o)

LIB=libkclib.a

all: $(OBJECTS)
	ar rcs $(LIB) $(OBJECTS)

%.o: %.c
	i686-elf-gcc -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c $< -o $@ -Iinclude

clean:
	rm -f *.o
	rm -f $(LIB)
#include "terminal.hpp"

extern "C" void kernel_main(void) 
{
	/* Initialize terminal interface */
	Terminal t;
 
	/* Newline support is left as an exercise. */
	t.writestring("Hello, kernel World!\n");
}

#include "terminal.hpp"

extern "C" void kernel_main(void) 
{
	/* Initialize terminal interface */
	Terminal t;
 
	/* Write a text as example */
	t.writestring("Hello, kernel World!\nHow are you doing?\nI added support for newlines! Sweet!");
}

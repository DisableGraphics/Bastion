// Commands 0xD0 and 0xD1 let you read and write the PS/2 Controller Output Port

/*
System reset (output)
WARNING always set to '1'. You need to pulse the reset line (e.g. using command 0xFE), and setting this bit to '0' can lock the computer up ("reset forever"). 
*/
#define RESET_BIT 0x1
// A20 gate (output) 
#define A20_GATE (1 << 1)
// Second PS/2 port clock (output, only if 2 PS/2 ports supported) 
#define PORT_2_CLOCK (1 << 2)
// Second PS/2 port data (output, only if 2 PS/2 ports supported) 
#define PORT_2_DATA (1 << 3)
// Output buffer full with byte from first PS/2 port (connected to IRQ1) 
#define OUT_BUF_PORT_1 (1 << 4)
// Output buffer full with byte from second PS/2 port (connected to IRQ12, only if 2 PS/2 ports supported) 
#define OUT_BUF_PORT_2 (1 << 5)
// First PS/2 port clock (output) 
#define CLOCK_PORT_1 (1 << 6)
// First PS/2 port data (output) 
#define DATA_PORT_1 (1 << 7 )
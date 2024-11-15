// Read "byte 0" from internal RAM
#define READ_BYTE_0 0x20
// Write next byte to "byte 0" of internal RAM (Controller Configuration Byte, see below)
#define WRITE_BYTE_0 0x60
// Disable second PS/2 port (only if 2 PS/2 ports supported) 
#define DISABLE_SECOND_PORT 0xA7
// Enable second PS/2 port (only if 2 PS/2 ports supported) 
#define ENABLE_SECOND_PORT 0xA8
// Test second PS/2 port (only if 2 PS/2 ports supported) 
#define TEST_SECOND_PORT 0xA9
// Test PS/2 Controller 
#define TEST_CONTROLLER 0xAA
// Test first PS/2 port 
#define TEST_FIRST_PORT 0xAB
// Diagnostic dump (read all bytes of internal RAM) 
#define DIAGNOSTIC_DUMP 0xAC
// Disable first PS/2 port 
#define DISABLE_FIRST_PORT 0xAD
// Enable first PS/2 port 
#define ENABLE_FIRST_PORT 0xAE
// Read controller input port 
#define READ_CONTROLLER_INPUT_PORT 0xC0
// Copy bits 0 to 3 of input port to status bits 4 to 7 
#define LOW_TO_HIGH_BITS 0xC1
// Copy bits 4 to 7 of input port to status bits 4 to 7 
#define HIGH_TO_HICH_BITS 0xC2
// Read Controller Output Port 
#define READ_CONTROLLER_OUTPUT_PORT 0xD0
/*
Write next byte to Controller Output Port
Note: Check if output buffer is empty first 
*/
#define WRITE_TO_CONTROLLER_OUTPUT_PORT 0xD1
/*
Write next byte to first PS/2 port output buffer (only if 2 PS/2 ports supported)
(makes it look like the byte written was received from the first PS/2 port) 
*/
#define WRITE_TO_PS2_1_OUT_BUFFER 0xD2
/*
Write next byte to second PS/2 port output buffer (only if 2 PS/2 ports supported)
(makes it look like the byte written was received from the second PS/2 port) 
*/
#define WRITE_TO_PS2_2_OUT_BUFFER 0xD3
/*
Write next byte to second PS/2 port input buffer (only if 2 PS/2 ports supported)
(sends next byte to the second PS/2 port) 
*/
#define WRITE_TO_PS2_2_IN_BUFFER 0xD4
/*
Pulse output line low for 6 ms. Bits 0 to 3 are used as a mask (0 = pulse line, 1 = don't pulse line) and correspond to 4 different output lines.
Note: Bit 0 corresponds to the "reset" line. The other output lines don't have a standard/defined purpose. 
Note 2: 0xF0 ~ 0xFF
*/
#define PULSE_LINE_LOW 0xF0
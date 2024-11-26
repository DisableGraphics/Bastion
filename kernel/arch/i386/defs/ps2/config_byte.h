// Interrupts for 1st port
#define FIRST_PORT_INTERRUPT 1
// Interrupts second port
#define SECOND_PORT_INTERRUPT (1 << 1)
// One if computer passed POST
#define SYSTEM_FLAG (1 << 2)
// Should be zero
#define ZERO (1 << 3)
// First port clock enable
#define FIRST_PORT_CLOCK (1 << 4)
// Second port clock enable
#define SECOND_PORT_CLOCK (1 << 5)
// First port translation
#define FIRST_PORT_TRANSLATION (1 << 6)
// Should be zero. Also, reference
#define ZERO_TWO (1 << 7)
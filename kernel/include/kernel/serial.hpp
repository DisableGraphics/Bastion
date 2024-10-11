int init_serial();

int serial_received();
char read_serial();

int is_transmit_empty();
extern "C" void write_serial(char a);
void serial_print(const char * str);
#pragma once
#ifdef __cplusplus
extern "C" { 
#endif
int init_serial(void);

int serial_received(void);
char read_serial(void);

int is_transmit_empty(void);

void write_serial(char a);
void serial_print(const char * str);
#ifdef __cplusplus
}
#endif
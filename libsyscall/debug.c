#include "debug.h"
#include <limits.h>
#include <stdarg.h>

#define PORT 0x3f8

int serial_init();

int debug_init(port_t port) {
	for(int i = 0; i < 6; i++) {
		const ipc_message_t init_msg = {
			.source = 0,
			.dest = port,
			.flags = IPC_FLAG_PORT_IO,
			.value0 = PORT + i
		};
		const int ret = sys_ipc_send(&init_msg);
		if(ret != EOK) {
			return ret;
		}
	}
	return 0;
}

void outb(uint16_t port, uint8_t val) {
	__asm__ volatile ("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

uint8_t inb(uint16_t port) {
	uint8_t ret;
	__asm__ volatile ("inb %w1, %b0"
				   : "=a"(ret)
				   : "Nd"(port)
				   : "memory");
	return ret;
}

int serial_is_transmit_empty() {
	return inb(PORT + 5) & 0x20;
}

void serial_write(char a) {
   while(!serial_is_transmit_empty());
   outb(PORT,a);
}

static int serial_print(const char *data, size_t length) {
	for (size_t i = 0; i < length; i++)
		serial_write(data[i]);
	return 1;
}

char * itoa(unsigned long value, char* str, int base) {
	char *rc;
	char *ptr;
	char *low;
	// Check for supported base.
	if (base < 2 || base > 36) {
		*str = '\0';
		return 0;
	}
	rc = ptr = str;
	// Set '-' for negative decimals.
	if (value < 0 && base == 10)
		*ptr++ = '-';
	if(base == 16)
	{
		*ptr++ = '0';
		*ptr++ = 'x';
	}
	// Remember where the numbers start.
	low = ptr;
	// The actual conversion.
	do {
		// Modulo is negative for negative value. This trick makes abs() unnecessary.
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + value % base];
		value /= base;
	} while (value);
	// Terminating the string.
	*ptr-- = '\0';
	// Invert the numbers.
	while (low < ptr) {
		char tmp = *low;
		*low++ = *ptr;
		*ptr-- = tmp;
	}
	return rc;
}

size_t strlen(const char* dst) {
	size_t ret = 0;
	while(*dst++) ret++;
	return ret;
}

int debug_write(const char *__restrict format, ...) {
	int written = 0;
	va_list parameters;
	va_start(parameters, format);

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				va_end(parameters);
				return -1;
			}
			serial_print(format, amount);
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			if (!maxrem) {
				va_end(parameters);
				return -1;
			}
			serial_print(&c, sizeof(char));
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				va_end(parameters);
				return -1;
			}
			serial_print(str, len);
			written += len;
		} else if(*format == 'd' || *format == 'i') {
			format++;
			int i = va_arg(parameters, int);
			char buf[32];
			itoa(i, buf, 10);
			size_t len = strlen(buf);
			if (maxrem < len) {
				va_end(parameters);
				return -1;
			}
			serial_print(buf, len);
			written += len;
		}
		else if(*format == 'l' && *(format + 1) == 'd'){
			format += 2;
			long i = va_arg(parameters, long);
			char buf[32];
			itoa(i, buf, 10);
			size_t len = strlen(buf);
			if (maxrem < len) {
				va_end(parameters);
				return -1;
			}
			serial_print(buf, len);
			written += len;

		} else if(*format == 'p' || *format == 'x') {
			format++;
			long i = (long)va_arg(parameters, void *);
			char buf[32];
			itoa(i, buf, 16);
			size_t len = strlen(buf);
			if (maxrem < len) {
				va_end(parameters);
				return -1;
			}
			serial_print(buf, len);
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				va_end(parameters);
				return -1;
			}
			serial_print(format, len);
			written += len;
			format += len;
		}
	}
	va_end(parameters);
	return written;
}

void puts(char* c) {
	serial_print(c, strlen(c));
}

void putc(char c) {
	serial_write(c);
}
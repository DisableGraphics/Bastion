#include <string.h>

int strcmp(const char* a, const char* b) {
	int ca, cb;
	do {
		ca = * (unsigned char *)a;
		cb = * (unsigned char *)b;
		a++;
		b++;
	} while (ca == cb && ca != '\0');
	return ca - cb;
}
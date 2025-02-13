#include <ctype.h>
#include <string.h>

int strcasecmp(const char* a, const char* b) {
	int ca, cb;
	do {
		ca = * (unsigned char *)a;
		cb = * (unsigned char *)b;
		ca = tolower(ca);
		cb = tolower(cb);
		a++;
		b++;
	} while (ca == cb && ca != '\0');
	return ca - cb;
}
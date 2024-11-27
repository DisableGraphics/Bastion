#include <ctype.h>

int isalpha(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

char tolower(char c) {
	return c + 32;
}

int isupper(char c) {
	return (c >= 'A' && c <= 'Z');
}
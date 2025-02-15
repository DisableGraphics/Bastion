#include <string.h>
#include <stdlib.h>

char** substr(const char* str, char delim, int* length) {
	if(!str || !length) return NULL;

	int count = 1;
    for (const char* temp = str; *temp; temp++) {
        if (*temp == delim) count++;
    }

	char** result = (char**)kmalloc(count * sizeof(char*));
    if (!result) return NULL;

	int index = 0;
    const char* start = str;
    for (const char* temp = str; ; temp++) {
        if (*temp == delim || *temp == '\0') {
            int len = temp - start;
            result[index] = (char*)kmalloc((len + 1) * sizeof(char));
            if (!result[index]) {
                // Free previously allocated memory on failure
                for (int i = 0; i < index; i++) kfree(result[i]);
                kfree(result);
                return NULL;
            }
            strncpy(result[index], start, len);
            result[index][len] = '\0'; // Null terminate the string
            index++;
            start = temp + 1;
        }
        if (*temp == '\0') break;
    }
    
    *length = count;
    return result;

	return result;
}
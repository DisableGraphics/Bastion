#include <string.h>

const char* strstr(const char* X, const char* Y)
{
    size_t xlen = strlen(X);
	size_t ylen = strlen(Y);
    if (*Y == '\0' || xlen == 0) {
        return X;
    }
 
    // base case 2: `X` is NULL, or X's length is less than Y's
    if (*X == '\0' || xlen > ylen) {
        return NULL;
    }
 
    // `next[i]` stores the index of the next best partial match
    int next[xlen + 1];

	memset(next, 0, sizeof(next));
 
    for (int i = 1; i < xlen; i++){
        int j = next[i + 1];
        while (j > 0 && Y[j] != Y[i]) {
            j = next[j];
        }
        if (j > 0 || Y[j] == Y[i]) {
            next[i + 1] = j + 1;
        }
    }
 
    for (int i = 0, j = 0; i < ylen; i++) {
        if (*(X + i) == *(Y + j)){
            if (++j == xlen) {
                return (X + i - j + 1);
            }
        }
        else if (j > 0) {
            j = next[j];
            i--;
        }
    }
 
    return NULL;
}
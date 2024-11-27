#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum CTYPE_MASK {
    DIGIT_MASK = 0x0001,
    XDIGT_MASK = 0x0002,
    LOWER_MASK = 0x0004,
    UPPER_MASK = 0x0008,
    ALPHA_MASK = 0x0010,
    PUNCT_MASK = 0x0020,
    SPACE_MASK = 0x0040,
    PRINT_MASK = 0x0080,
    CNTRL_MASK = 0x0100,
    BLANK_MASK = 0x0200,

    ALNUM_MASK = ALPHA_MASK | DIGIT_MASK,
    GRAPH_MASK = ALNUM_MASK | PUNCT_MASK
};

extern unsigned short ctype_bits[];

#define isalpha(c)  ((ctype_bits+1)[c] & ALPHA_MASK)
#define isupper(c)  ((ctype_bits+1)[c] & UPPER_MASK)
#define islower(c)  ((ctype_bits+1)[c] & LOWER_MASK)
#define isalnum(c)  ((ctype_bits+1)[c] & ALNUM_MASK)
#define isgraph(c)  ((ctype_bits+1)[c] & GRAPH_MASK)
#define isprint(c)  ((ctype_bits+1)[c] & PRINT_MASK)
#define iscntrl(c)  ((ctype_bits+1)[c] & CNTRL_MASK)
#define isdigit(c)  ((ctype_bits+1)[c] & DIGIT_MASK)
#define isblank(c)  ((ctype_bits+1)[c] & BLANK_MASK)
#define isspace(c)  ((ctype_bits+1)[c] & SPACE_MASK)
#define ispunct(c)  ((ctype_bits+1)[c] & PUNCT_MASK)
#define isxdigit(c) ((ctype_bits+1)[c] & XDIGT_MASK)

char tolower(char c);
char toupper(char c);

#ifdef __cplusplus
}
#endif
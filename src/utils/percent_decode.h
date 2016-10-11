#ifndef PERCENT_DECODE_H
#define PERCENT_DECODE_H


/* Function from busybox.
 * Use strict=1 if you process input from untrusted source:
 * it will return NULL on invalid %xx (bad hex chars)
 * and str + 1 if decoded char is / or NUL.
 * In non-strict mode, it always succeeds (returns str),
 * and also it additionally decoded '+' to space.
 */
#ifdef __cplusplus
extern "C"
#endif
char *percent_decode_in_place(char *str, int strict);

#endif

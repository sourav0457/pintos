// For Fixed Point Arithematic Operations
// fixed point is a 32-bit integer in the format 17.14
// x & y are fixed point numbers, n is an integer
// to be included in timer.c and thread.c

#ifndef THREADS_FIXEDPOINT_H
#define THREADS_FIXEDPOINT_H
#define F (1 << 14)

// Conversions
int int_to_fp (int n) {return n * F;}
int fp_to_int_towards_zero (int n) {return n / F;}
int fp_to_int_towards_nearest  (int n) {return (n >= 0 ? (n + F / 2) / F : (n - F / 2) / F);}

// Arithematic Operations
int add_fp_fp (int x, int y) {return x + y;}
int sub_fp_fp (int x, int y) {return x - y;}
int mul_fp_fp (int x, int y) {return ((int64_t) x) * y / F;}
int div_fp_fp (int x, int y) {return ((int64_t) x) * F / y;}
int add_fp_int (int x, int n) {return x + n * F;}
int sub_fp_int (int x, int n) {return x - n * F;}
int mul_fp_int (int x, int n) {return x * n;}
int div_fp_int (int x, int n) {return x / n;}

#endif
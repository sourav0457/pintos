// For Fixed Point Arithematic Operations
// fixed point is a 32-bit integer in the format 17.14
// x & y are fixed point numbers, n is an integer
// to be included in timer.c and thread.c

#ifndef THREADS_FIXEDPOINT_H
#define THREADS_FIXEDPOINT_H
#define F (1 << 14)

// Conversions
#define int_to_fp (n) ((n) * (F))
#define fp_to_int_towards_zero (n) ((n) / (F))
#define fp_to_int_towards_nearest  (n) ((n) >= 0) ? (((n) + (F / 2)) / (F)) : (((n) - (F / 2)) / (F))

// Arithematic Operations
#define add_fp_fp (x, y) ((x) + (y))
#define sub_fp_fp (x, y) ((x) - (y))

#define mul_fp_fp (x, y) ((((int64_t) x) * (y))/ (F))
#define div_fp_fp (x, y) ((((int64_t) x) * (F)) / (y))
#define add_fp_int (x, n) ((x) + ((n) * F))
#define sub_fp_int (x, n) ((x) - ((n) * F))
#define mul_fp_int (x, n) ((x) * (n))
#define div_fp_int (x, n) ((x) / (n))

#endif
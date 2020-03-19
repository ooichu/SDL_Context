/* 
 * Title: vec.h
 * Autor: @ooichu
 * Desc: Simple generic math vector library.
 * Note: Use vec_t to declare new vector type.
 * Note: Avoid things like this: v = vec_Add(vec_Mul(a, b), c), then you catch error.
 * Note: Define __VEC_STRICT_CAST__ for disable auto casting result to left operand type. 
 * */

#ifndef __VEC_H__
#define __VEC_H__

#include <math.h> /* for sqrt(), sin(), cos() */

#define __ABS__(x) ((x) < 0 ? -(x) : (x))

#define vec_t(t) struct { t x, y, z; }

/* operations that returns scalar */
#define vec_Dot(a, b)	((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)
#define vec_Length(v)	(sqrt((v).x * (v).x + (v).y * (v).y + (v).z * (v).z))
#define vec_Normalize(v) ()

/* operations that returns bool */
#define vec_Eq(a, b) ((a).x == (b).x && (a).y == (b).y && (a).z == (b).z)

/* operations that returns vector */
#if (defined(__GNUC__) || defined(__GNUG__)) && !defined(__VEC_STRICT_CAST__) 

#define vec_Add(a, b) 	(typeof(a)) {(a).x + (b).x, (a).y + (b).y, (a).z + (b).z}
#define vec_Sub(a, b) 	(typeof(a)) {(a).x - (b).x, (a).y - (b).y, (a).z - (b).z}
#define vec_Mul(a, v) 	(typeof(a)) {(a).x * v, (a).y * v, (a).z * v}
#define vec_Div(v, x) 	(typeof(a)) {(v).x / x, (v).y / x, (v).z / x}
#define vec_Abs(v)		(typeof(a)) {__ABS__(v.x), __ABS__(v.y), __ABS__(v.z)}
#define vec_Cross(a, b)	(typeof(a)) {(a).y * (b).z - (a).z * (b).y, (a).x * (b).z - (a).z * (b).x, (a).x * (b).y - (a).y * (b).x}
#if 0
#define vec_RotX(v, a) \
	do { \
		typeof((v).x) __x = (v).x, __y = (v).y; \
		double s = sin(a), c = cos(a); \
		(v).x = c * __x - s * __y; \
		(v).y = s * __x + c * __y; \
	} while(0)
#define vec_RotY(v, a) \
	do { \
		typeof((v).x) __x = (v).x, __z = (v).z; \
		double s = sin(a), c = cos(a); \
		(v).x = c * __x - s * __z; \
		(v).z = s * __x + c * __z; \
	} while(0)
#define vec_RotZ(v, a)
#endif

#else // (defined(__GNUC__) || defined(__GNUG__)) && !defined(__VEC_STRICT_CAST__)

#define vec_Add(t, a, b)	((t)) {(a).x + (b).x, (a).y + (b).y, (a).z + (b).z}
#define vec_Sub(t, a, b)	((t)) {(a).x - (b).x, (a).y - (b).y, (a).z - (b).z}
#define vec_Mul(t, a, v)	((t)) {(a).x * v, (a).y * v, (a).z * v}
#define vec_Div(t, v, x)	((t)) {(v).x / x, (v).y / x, (v).z / x}
#define vec_Abs(t, v)		((t)) {__ABS__(v.x), __ABS__(v.y), __ABS__(v.z)}
#define vec_Cross(t, a, b)	((t)) {(a).y * (b).z - (a).z * (b).y, (a).x * (b).z - (a).z * (b).x, (a).x * (b).y - (a).y * (b).x}
#if 0
#define vec_RotX(t, v, a) \
	do { \
		t __x = (v).x, __y = (v).y; \
		double s = sin(a), c = cos(a); \
		(v).x = c * __x - s * __y; \
		(v).y = s * __x + c * __y; \
	} while(0)
#define vec_RotY(t, v, a)
#define vec_RotZ(t, v, a)
#endif

#endif // (defined(__GNUC__) || defined(__GNUG__)) && !defined(__VEC_STRICT_CAST__) 

#undef __ABS__

/* rotation */

#ifdef __VEC_STRICT_CAST__
#undef __VEC_STRICT_CAST__
#endif // __VEC_STRICT_CAST__

typedef vec_t(int) vec_int;
typedef vec_t(char) vec_char;
typedef vec_t(float) vec_float;
typedef vec_t(double) vec_double;

#endif // __VEC_H__


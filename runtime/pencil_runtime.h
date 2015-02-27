/*
 * Copyright (c) 2014, ARM Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef PENCIL_RUNTIME_H
#define PENCIL_RUNTIME_H

#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum PENCIL_INIT_FLAG
{
    PENCIL_TARGET_DEVICE_GPU_ONLY,
    PENCIL_TARGET_DEVICE_CPU_ONLY,
    PENCIL_TARGET_DEVICE_GPU_THEN_CPU,
    PENCIL_TARGET_DEVICE_CPU_THEN_GPU,
    PENCIL_TARGET_DEVICE_DYNAMIC,

	PENCIL_CPU_PROFILING_ENABLED = 1<<3, // Skip the first three bits used to select the target device preference
	PENCIL_GPU_PROFILING_ENABLED = 1<<4,
	PENCIL_PROFILING_ENABLED = PENCIL_CPU_PROFILING_ENABLED | PENCIL_GPU_PROFILING_ENABLED,
	PENCIL_BLOCKING_ENABLED = 1 << 5,
};

/** Memory allocation. Can be used in the same way, malloc is used.  */
extern void * pencil_alloc (size_t size);

/** Free memory allocated by pencil_alloc.  */
extern void pencil_free (void * ptr);

/** Initialize PENCIL runtime.  */
extern void pencil_init (enum PENCIL_INIT_FLAG flag);

/** Release PENCIL runtime.  */
extern void pencil_shutdown (void);

/** Dump statictics to stdout */
extern void pencil_dump_stats (void);
extern void pencil_reset_stats (void);

/** smaller of two numbers, 
    potentially called by ppcg generated code */
static inline int __ppcg_min(int a, int b)
{
    if (a < b)
        return a;
    return b;
}

/** greater of two numbers, 
    potentially called by ppcg generated code */
static inline int __ppcg_max(int a, int b)
{
    if (a > b)
        return a;
    return b;
}

/** floored division (round to negative infinity), 
    potentially called by ppcg generated code      */
static inline int __ppcg_floord(int n, int d)
{
    if (n<0)
        return -((-n+d-1)/d);
    return n/d;
}




// Implementations for pencil_prototypes.h
// Required because ppcg may choose to generate CPU code for a PENCIL function
// pencil-optimizer will replicate these declarations, i.e. we cannot define them as macros (like in pencil_lib.h)

// NOTE: Is there any system in the naming of these functions?

static inline char  __attribute__((const,always_inline)) babs(char x) { return abs(x); }
static inline short __attribute__((const,always_inline))  sabs(short x) { return abs(x); }
static inline int   __attribute__((const,always_inline))  iabs(int x) { return abs(x); }
//static inline long labs(long x) { return labs(x); } // exact replication
static inline unsigned char  __attribute__((const,always_inline)) ubabs(unsigned char x) { return abs(x); }
static inline unsigned short __attribute__((const,always_inline)) usabs(unsigned short x) { return abs(x); }
static inline unsigned int   __attribute__((const,always_inline)) uabs(unsigned int x) { return abs(x); }
static inline unsigned long  __attribute__((const,always_inline)) ulabs(unsigned long x) { return abs(x); }
static inline char  __attribute__((const,always_inline))  bmax(char a, char b) { return (a>=b) ? a : b; }
static inline short __attribute__((const,always_inline))  smax(short a, short b) { return (a>=b) ? a : b; }
static inline int   __attribute__((const,always_inline))  imax(int a, int b) { return (a>=b) ? a : b; }
static inline long  __attribute__((const,always_inline))  lmax(long a, long b) { return (a>=b) ? a : b; }
static inline unsigned char  __attribute__((const,always_inline))  ubmax(unsigned char a, unsigned char b) { return (a>=b) ? a : b; }
static inline unsigned short __attribute__((const,always_inline))  usmax(unsigned short a, unsigned short b) { return (a>=b) ? a : b; }
static inline unsigned int   __attribute__((const,always_inline))  umax(unsigned int a, unsigned int b) { return (a>=b) ? a : b; }
static inline unsigned long  __attribute__((const,always_inline))  ulmax(unsigned long a, unsigned long b) { return (a>=b) ? a : b; }
static inline char  __attribute__((const,always_inline)) bmin(char a, char b) { return (a<=b) ? a : b; }
static inline short __attribute__((const,always_inline)) smin(short a, short b) { return (a<=b) ? a : b; }
static inline int   __attribute__((const,always_inline)) imin(int a, int b) { return (a<=b) ? a : b; }
static inline long  __attribute__((const,always_inline)) lmin(long a, long b) { return (a<=b) ? a : b; }
static inline unsigned char  __attribute__((const,always_inline)) ubmin(unsigned char a, unsigned char b) { return (a<=b) ? a : b; }
static inline unsigned short __attribute__((const,always_inline)) usmin(unsigned short a, unsigned short b) { return (a<=b) ? a : b; }
static inline unsigned int   __attribute__((const,always_inline)) umin(unsigned int a, unsigned int b) { return (a<=b) ? a : b; }
static inline unsigned long  __attribute__((const,always_inline)) ulmin(unsigned long a, unsigned long b) { return (a<=b) ? a : b; }
static inline float  __attribute__((const,always_inline)) clampf(float val, float min, float max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline double __attribute__((const,always_inline)) clampd(double val, double min, double max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline float  __attribute__((const,always_inline)) degreesf(float rad) { return (180/M_PI) * rad; }
static inline double __attribute__((const,always_inline)) degreesd(double rad) { return (180/M_PI) * rad; }
static inline float  __attribute__((const,always_inline)) mixf(float x, float y, float a) { return x + (y-x)*a; }
static inline double __attribute__((const,always_inline)) mixd(double x, double y, double a) { return x + (y-x)*a; }
static inline float  __attribute__((const,always_inline)) radiansf(float deg) { return (M_PI/180) * deg; }
static inline double __attribute__((const,always_inline)) radiansd(double deg) { return (M_PI/180) * deg; }
static inline float  __attribute__((const,always_inline)) signf(float x) { if(isnan(x)) return 0; if (x>0) return 1; if (x<0) return -1; return copysign(x,0); }
static inline double __attribute__((const,always_inline)) signd(double x) { if(isnan(x)) return 0; if (x>0) return 1; if (x<0) return -1; return copysign(x,0); }
static inline float  __attribute__((const,always_inline)) stepf(float edge, float x) { return x<edge ? 0 : 1; }
static inline double __attribute__((const,always_inline)) stepd(double edge, double x) { return x<edge ? 0 : 1; }
//float fmodf(float x) __attribute__((const,always_inline)); // GCC: warning: conflicting types for built-in function 'fmodf
//static inline double fmodd(double x, double y) { return fmod(x, y); }
static inline double __attribute__((const,always_inline)) remainderd(double a, double b) { return remainder(a,b); }
static inline double __attribute__((const,always_inline)) remquod(double x, double y, int* quo) { return remquo(x,y,quo); }
static inline double __attribute__((const,always_inline)) fmad(double a, double b, double c) { return fma(a,b,c); }
static inline double __attribute__((const,always_inline)) madf(double a, double b, double c) { return a*b+c; }
static inline double __attribute__((const,always_inline)) madd(double a, double b, double c) { return a*b+c; }
static inline double __attribute__((const,always_inline)) fdimd(double x, double y) { return fdim(x,y); }
static inline double __attribute__((const,always_inline)) fmaxd(double a, double b) { return fmax(a,b); }
static inline double __attribute__((const,always_inline)) fmind(double a, double b) { return fmin(a,b); }
static inline double __attribute__((const,always_inline)) fabsd(double x) { return fabs(x); }
static inline double __attribute__((const,always_inline)) expd(double x) { return exp(x); }
static inline double __attribute__((const,always_inline)) exp2d(double x) { return exp2(x); }
//float exp10f(float x) __attribute__((const,always_inline));
//double exp10d(double x) __attribute__((const,always_inline));
static inline double __attribute__((const,always_inline)) expm1d(double x) { return expm1(x); }
static inline double __attribute__((const,always_inline)) logd(double x) { return log(x); }
static inline double __attribute__((const,always_inline)) log2d(double x) { return log2(x); }
static inline double __attribute__((const,always_inline)) log10d(double x) { return log10(x); }
static inline double __attribute__((const,always_inline)) log1pd(double x) { return log1p(x); }
static inline double __attribute__((const,always_inline)) powd(double x, double y) { return pow(x,y); }
static inline float __attribute__((const, always_inline)) pownf(float x, int y) { return powf(x,y); }
static inline double __attribute__((const, always_inline)) pownd(double x, int y) { return pow(x,y); }
static inline float __attribute__((const, always_inline)) powrf(float x, float y) { return powf(x,y); }
static inline double __attribute__((const, always_inline)) powrd(double x, double y) { return pow(x,y); }
static inline double __attribute__((const,always_inline)) sqrtd(double x) { return sqrt(x); }
static inline float __attribute__((const, always_inline)) rsqrtf(float x) { return 1/sqrtf(x); }
static inline double __attribute__((const, always_inline)) rsqrtd(double x) { return 1/sqrt(x); }
static inline double __attribute__((const,always_inline)) cbrtd(double x) { return cbrt(x); }
static inline float __attribute__((const, always_inline)) rootnf(float x, int n) { return powf(x,(float)1.0/(float)n); }
static inline double __attribute__((const, always_inline)) rootnd(double x, int n) { return pow(x,(double)1.0/(double)n); }
static inline double __attribute__((const,always_inline)) hypotd(double x, double y) { return hypot(x,y); }
static inline double __attribute__((const,always_inline)) sind(double x) { return sin(x); }
static inline double __attribute__((const,always_inline)) cosd(double x) { return cos(x); }
static inline double __attribute__((const,always_inline)) tand(double x) { return tan(x); }
static inline double __attribute__((const,always_inline)) asind(double x) { return asin(x); }
static inline double __attribute__((const,always_inline)) acosd(double x) { return acos(x); }
static inline double __attribute__((const,always_inline)) atand(double x) { return atan(x); }
static inline double __attribute__((const,always_inline)) atan2d(double x, double y) { return atan2(x,y); }
static inline float  __attribute__((const, always_inline)) sinpif(float x) { return sinf((float)M_PI*x); }
static inline double __attribute__((const, always_inline)) sinpid(double x) { return sin((double)M_PI*x); }
static inline float  __attribute__((const, always_inline)) cospif(float x) { return cosf((float)M_PI*x); }
static inline double __attribute__((const, always_inline)) cospid(double x) { return cos((double)M_PI*x); }
static inline float  __attribute__((const, always_inline)) tanpif(float x) { return tanf((float)M_PI*x); }
static inline double __attribute__((const, always_inline)) tanpid(double x) { return tan((double)M_PI*x); }
static inline float  __attribute__((const, always_inline)) asinpif(float x) { return asinf(x)/(float)M_PI; }
static inline double __attribute__((const, always_inline)) asinpid(double x) { return asin(x)/(double)M_PI; }
static inline float  __attribute__((const, always_inline)) acospif(float x) { return acosf(x)/(float)M_PI; }
static inline double __attribute__((const, always_inline)) acospid(double x) { return acos(x)/(double)M_PI; }
static inline float  __attribute__((const, always_inline)) atanpif(float x) { return atanf(x)/(float)M_PI; }
static inline double __attribute__((const, always_inline)) atanpid(double x) { return atan(x)/(double)M_PI; }
static inline float  __attribute__((const, always_inline)) atan2pif(float x, float y) { return atan2f(x,y)/(float)M_PI; }
static inline double __attribute__((const, always_inline)) atan2pid(double x, double y) { return atan2(x,y)/(double)M_PI; }
static inline double __attribute__((const,always_inline)) sinhd(double x) { return sinh(x); }
static inline double __attribute__((const,always_inline)) coshd(double x) { return cosh(x); }
static inline double __attribute__((const,always_inline)) tanhd(double x) { return tanh(x); }
static inline double __attribute__((const,always_inline)) asinhd(double x) { return asinh(x); }
static inline double __attribute__((const,always_inline)) acoshd(double x) { return acosh(x); }
static inline double __attribute__((const,always_inline)) atanhd(double x) { return atanh(x); }
static inline double __attribute__((const,always_inline)) erfd(double x) { return erf(x); }
static inline double __attribute__((const,always_inline)) erfcd(double x) { return erfc(x); }
static inline double __attribute__((const,always_inline)) tgammad(double x) { return tgamma(x); }
static inline double __attribute__((const,always_inline)) lgammad(double x) { return lgamma(x); }
static inline double __attribute__((const,always_inline)) lgamma_rd(double x, int* signp) { return lgamma_r(x, signp); }
static inline double __attribute__((const,always_inline)) ceild(double x) { return ceil(x); }
static inline double __attribute__((const,always_inline)) floord(double x) { return floor(x); }
static inline double __attribute__((const,always_inline)) truncd(double x) { return trunc(x); }
static inline double __attribute__((const,always_inline)) roundd(double x) { return round(x); }
static inline double __attribute__((const,always_inline)) logbd(double x) { return logb(x); }
static inline double __attribute__((const,always_inline)) nextafterd(double from, double to) { return nextafter(from, to); }
static inline double __attribute__((const,always_inline)) copysignd(double x, double y) { return copysign(x, y); }
static inline char  __attribute__((const, always_inline)) bclampi(char val, char min, char max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline short __attribute__((const, always_inline)) sclampi(short val, short min, short max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline int   __attribute__((const, always_inline)) iclampi(int val, int min, int max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline long  __attribute__((const, always_inline)) lclampi(long val, long min, long max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline unsigned char  __attribute__((const, always_inline)) ubclampi(unsigned char val, unsigned char min, unsigned char max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline unsigned short __attribute__((const, always_inline)) usclampi(unsigned short val, unsigned short min, unsigned short max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline unsigned int   __attribute__((const, always_inline)) uclampi(unsigned int val, unsigned int min, unsigned int max) { return (val<min) ? min : ((val>max) ? max : val); }
static inline unsigned long  __attribute__((const, always_inline)) ulclampi(unsigned long val, unsigned long min, unsigned long max) { return (val<min) ? min : ((val>max) ? max : val); }

#ifdef __cplusplus
}
#endif

#endif

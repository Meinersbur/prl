/*
 * Copyright (c) 2014, Realeyes
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
 * 
 * Note: this is a generated file. Do not modify.
 */
#ifndef PENCIL_LIB_H
#define PENCIL_LIB_H
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
inline char __attribute__((const)) bmax(char a, char b) { return a>=b ? a : b; }
inline short __attribute__((const)) smax(short a, short b) { return a>=b ? a : b; }
inline int __attribute__((const)) max(int a, int b) { return a>=b ? a : b; }
inline long __attribute__((const)) lmax(long a, long b) { return a>=b ? a : b; }
inline unsigned char __attribute__((const)) ubmax(unsigned char a, unsigned char b) { return a>=b ? a : b; }
inline unsigned short __attribute__((const)) usmax(unsigned short a, unsigned short b) { return a>=b ? a : b; }
inline unsigned int __attribute__((const)) umax(unsigned int a, unsigned int b) { return a>=b ? a : b; }
inline unsigned long __attribute__((const)) ulmax(unsigned long a, unsigned long b) { return a>=b ? a : b; }
inline char __attribute__((const)) bmin(char a, char b) { return a<=b ? a : b; }
inline short __attribute__((const)) smin(short a, short b) { return a<=b ? a : b; }
inline int __attribute__((const)) min(int a, int b) { return a<=b ? a : b; }
inline long __attribute__((const)) lmin(long a, long b) { return a<=b ? a : b; }
inline unsigned char __attribute__((const)) ubmin(unsigned char a, unsigned char b) { return a<=b ? a : b; }
inline unsigned short __attribute__((const)) usmin(unsigned short a, unsigned short b) { return a<=b ? a : b; }
inline unsigned int __attribute__((const)) umin(unsigned int a, unsigned int b) { return a<=b ? a : b; }
inline unsigned long __attribute__((const)) ulmin(unsigned long a, unsigned long b) { return a<=b ? a : b; }
inline float __attribute__((const)) clampf(float val, float min, float max) { return (val<min) ? min : ((val>max) ? max : val); }
inline double __attribute__((const)) clamp(double val, double min, double max) { return (val<min) ? min : ((val>max) ? max : val); }
inline float __attribute__((const)) degreesf(float rad) { return (__typeof__(rad))(180/M_PI) * rad; }
inline double __attribute__((const)) degrees(double rad) { return (__typeof__(rad))(180/M_PI) * rad; }
inline float __attribute__((const)) mixf(float x, float y, float a) { return x + (y-x)*a; }
inline double __attribute__((const)) mix(double x, double y, double a) { return x + (y-x)*a; }
inline float __attribute__((const)) radiansf(float deg) { return (__typeof__(deg))(M_PI/180) * deg; }
inline double __attribute__((const)) radians(double deg) { return (__typeof__(deg))(M_PI/180) * deg; }
inline float __attribute__((const)) signf(float x) { if(isnan(x)) return 0.0; if (x>0.0) return 1.0; if (x<0.0) return -1.0; return copysign(x,0.0); }
inline double __attribute__((const)) sign(double x) { if(isnan(x)) return 0.0; if (x>0.0) return 1.0; if (x<0.0) return -1.0; return copysign(x,0.0); }
inline float __attribute__((const)) stepf(float edge, float x) { return x<edge ? (__typeof__(x))0.0 : (__typeof__(x))1.0; }
inline double __attribute__((const)) step(double edge, double x) { return x<edge ? (__typeof__(x))0.0 : (__typeof__(x))1.0; }
inline float __attribute__((const)) madf(float a, float b, float c) { return a*b+c; }
inline double __attribute__((const)) mad(double a, double b, double c) { return a*b+c; }
inline float __attribute__((const)) exp10f(float x) { return pow(10, x); }
inline double __attribute__((const)) exp10(double x) { return pow(10, x); }
inline float __attribute__((const)) pownf(float x, int y) { return pow(x,y); }
inline double __attribute__((const)) pown(double x, int y) { return pow(x,y); }
inline float __attribute__((const)) powrf(float x, float y) { return pow(x,y); }
inline double __attribute__((const)) powr(double x, double y) { return pow(x,y); }
inline float __attribute__((const)) rsqrtf(float x) { return 1/sqrt(x); }
inline double __attribute__((const)) rsqrt(double x) { return 1/sqrt(x); }
inline float __attribute__((const)) rootnf(float x, int n) { return pow(x,1.0/n); }
inline double __attribute__((const)) rootn(double x, int n) { return pow(x,1.0/n); }
inline float __attribute__((const)) sinpif(float x) { return sin(M_PI*x); }
inline double __attribute__((const)) sinpi(double x) { return sin(M_PI*x); }
inline float __attribute__((const)) cospif(float x) { return cos(M_PI*x); }
inline double __attribute__((const)) cospi(double x) { return cos(M_PI*x); }
inline float __attribute__((const)) tanpif(float x) { return tan(M_PI*x); }
inline double __attribute__((const)) tanpi(double x) { return tan(M_PI*x); }
inline float __attribute__((const)) asinpif(float x) { return asin(x)/M_PI; }
inline double __attribute__((const)) asinpi(double x) { return asin(x)/M_PI; }
inline float __attribute__((const)) acospif(float x) { return acos(x)/M_PI; }
inline double __attribute__((const)) acospi(double x) { return acos(x)/M_PI; }
inline float __attribute__((const)) atanpif(float x) { return atan(x)/M_PI; }
inline double __attribute__((const)) atanpi(double x) { return atan(x)/M_PI; }
inline float __attribute__((const)) atan2pif(float x, float y) { return atan2(x,y)/M_PI; }
inline double __attribute__((const)) atan2pi(double x, double y) { return atan2(x,y)/M_PI; }
inline char __attribute__((const)) bclampi(char val, char min, char max) { return (val<min) ? min : ((val>max) ? max : val); }
inline short __attribute__((const)) sclampi(short val, short min, short max) { return (val<min) ? min : ((val>max) ? max : val); }
inline int __attribute__((const)) clampi(int val, int min, int max) { return (val<min) ? min : ((val>max) ? max : val); }
inline long __attribute__((const)) lclampi(long val, long min, long max) { return (val<min) ? min : ((val>max) ? max : val); }
inline unsigned char __attribute__((const)) ubclampi(unsigned char val, unsigned char min, unsigned char max) { return (val<min) ? min : ((val>max) ? max : val); }
inline unsigned short __attribute__((const)) usclampi(unsigned short val, unsigned short min, unsigned short max) { return (val<min) ? min : ((val>max) ? max : val); }
inline unsigned int __attribute__((const)) uclampi(unsigned int val, unsigned int min, unsigned int max) { return (val<min) ? min : ((val>max) ? max : val); }
inline unsigned long __attribute__((const)) ulclampi(unsigned long val, unsigned long min, unsigned long max) { return (val<min) ? min : ((val>max) ? max : val); }
#endif

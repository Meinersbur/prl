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
inline char __attribute__((const)) bmin(char a, char b) { return a <= b ? a : b; }
inline short __attribute__((const)) smin(short a, short b) { return a <= b ? a : b; }
inline int __attribute__((const)) min(int a, int b) { return a <= b ? a : b; }
inline long __attribute__((const)) lmin(long a, long b) { return a <= b ? a : b; }
inline unsigned char __attribute__((const)) ubmin(unsigned char a, unsigned char b) { return a <= b ? a : b; }
inline unsigned short __attribute__((const)) usmin(unsigned short a, unsigned short b) { return a <= b ? a : b; }
inline unsigned int __attribute__((const)) umin(unsigned int a, unsigned int b) { return a <= b ? a : b; }
inline unsigned long __attribute__((const)) ulmin(unsigned long a, unsigned long b) { return a <= b ? a : b; }
inline char __attribute__((const)) bmax(char a, char b) { return a >= b ? a : b; }
inline short __attribute__((const)) smax(short a, short b) { return a >= b ? a : b; }
inline int __attribute__((const)) max(int a, int b) { return a >= b ? a : b; }
inline long __attribute__((const)) lmax(long a, long b) { return a >= b ? a : b; }
inline unsigned char __attribute__((const)) ubmax(unsigned char a, unsigned char b) { return a >= b ? a : b; }
inline unsigned short __attribute__((const)) usmax(unsigned short a, unsigned short b) { return a >= b ? a : b; }
inline unsigned int __attribute__((const)) umax(unsigned int a, unsigned int b) { return a >= b ? a : b; }
inline unsigned long __attribute__((const)) ulmax(unsigned long a, unsigned long b) { return a >= b ? a : b; }
inline float __attribute__((const)) clampf(float val, float min, float max) { return (val < min) ? min : ((val > max) ? max : val); }
inline double __attribute__((const)) clamp(double val, double min, double max) { return (val < min) ? min : ((val > max) ? max : val); }
inline char __attribute__((const)) bclampi(char val, char min, char max) { return (val < min) ? min : ((val > max) ? max : val); }
inline short __attribute__((const)) sclampi(short val, short min, short max) { return (val < min) ? min : ((val > max) ? max : val); }
inline int __attribute__((const)) clampi(int val, int min, int max) { return (val < min) ? min : ((val > max) ? max : val); }
inline long __attribute__((const)) lclampi(long val, long min, long max) { return (val < min) ? min : ((val > max) ? max : val); }
inline unsigned char __attribute__((const)) ubclampi(unsigned char val, unsigned char min, unsigned char max) { return (val < min) ? min : ((val > max) ? max : val); }
inline unsigned short __attribute__((const)) usclampi(unsigned short val, unsigned short min, unsigned short max) { return (val < min) ? min : ((val > max) ? max : val); }
inline unsigned int __attribute__((const)) uclampi(unsigned int val, unsigned int min, unsigned int max) { return (val < min) ? min : ((val > max) ? max : val); }
inline unsigned long __attribute__((const)) ulclampi(unsigned long val, unsigned long min, unsigned long max) { return (val < min) ? min : ((val > max) ? max : val); }
inline float __attribute__((const)) atan2pif(float x, float y) { return atan2(x,y)/M_PI; }
inline double __attribute__((const)) atan2pi(double x, double y) { return atan2(x,y)/M_PI; }
inline float __attribute__((const)) mixf(float x, float y, float a) { return x + (y-x)*a; }
inline double __attribute__((const)) mix(double x, double y, double a) { return x + (y-x)*a; }
#endif

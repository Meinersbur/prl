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
#ifndef PENCIL_OPENCL_H
#define PENCIL_OPENCL_H
#define fminf(a, b) (fmin)((float)(a), (float)(b))
#define fmin(a, b) (fmin)((double)(a), (double)(b))
#define bmin(a, b) (min)((char)(a), (char)(b))
#define smin(a, b) (min)((short)(a), (short)(b))
#define min(a, b) (min)((int)(a), (int)(b))
#define lmin(a, b) (min)((long)(a), (long)(b))
#define ubmin(a, b) (min)((unsigned char)(a), (unsigned char)(b))
#define usmin(a, b) (min)((unsigned short)(a), (unsigned short)(b))
#define umin(a, b) (min)((unsigned int)(a), (unsigned int)(b))
#define ulmin(a, b) (min)((unsigned long)(a), (unsigned long)(b))
#define fmaxf(a, b) (fmax)((float)(a), (float)(b))
#define fmax(a, b) (fmax)((double)(a), (double)(b))
#define bmax(a, b) (max)((char)(a), (char)(b))
#define smax(a, b) (max)((short)(a), (short)(b))
#define max(a, b) (max)((int)(a), (int)(b))
#define lmax(a, b) (max)((long)(a), (long)(b))
#define ubmax(a, b) (max)((unsigned char)(a), (unsigned char)(b))
#define usmax(a, b) (max)((unsigned short)(a), (unsigned short)(b))
#define umax(a, b) (max)((unsigned int)(a), (unsigned int)(b))
#define ulmax(a, b) (max)((unsigned long)(a), (unsigned long)(b))
#define clampf(val, min, max) (clamp)((float)(val), (float)(min), (float)(max))
#define clamp(val, min, max) (clamp)((double)(val), (double)(min), (double)(max))
inline char bclampi(char val, char min, char max) { return (val < min) ? min : (val > max) ? max : val; }
inline short sclampi(short val, short min, short max) { return (val < min) ? min : (val > max) ? max : val; }
inline int clampi(int val, int min, int max) { return (val < min) ? min : (val > max) ? max : val; }
inline long lclampi(long val, long min, long max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned char ubclampi(unsigned char val, unsigned char min, unsigned char max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned short usclampi(unsigned short val, unsigned short min, unsigned short max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned int uclampi(unsigned int val, unsigned int min, unsigned int max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned long ulclampi(unsigned long val, unsigned long min, unsigned long max) { return (val < min) ? min : (val > max) ? max : val; }
#define tanf(x) (tan)((float)(x))
#define tan(x) (tan)((double)(x))
#define atanf(x) (atan)((float)(x))
#define atan(x) (atan)((double)(x))
#define atan2f(x, y) (atan2)((float)(x), (float)(y))
#define atan2(x, y) (atan2)((double)(x), (double)(y))
#define hypotf(x, y) (hypot)((float)(x), (float)(y))
#define hypot(x, y) (hypot)((double)(x), (double)(y))
#define expf(x) (exp)((float)(x))
#define exp(x) (exp)((double)(x))
#define ceilf(x) (ceil)((float)(x))
#define ceil(x) (ceil)((double)(x))
#define floorf(x) (floor)((float)(x))
#define floor(x) (floor)((double)(x))
#define mixf(x, y, a) (mix)((float)(x), (float)(y), (float)(a))
#define mix(x, y, a) (mix)((double)(x), (double)(y), (double)(a))
#endif

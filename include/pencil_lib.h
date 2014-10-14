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
#define M_PI 3.14159265358979323846
inline char bmin(char a, char b) { return a <= b ? a : b; }
inline short smin(short a, short b) { return a <= b ? a : b; }
inline int min(int a, int b) { return a <= b ? a : b; }
inline long lmin(long a, long b) { return a <= b ? a : b; }
inline unsigned char ubmin(unsigned char a, unsigned char b) { return a <= b ? a : b; }
inline unsigned short usmin(unsigned short a, unsigned short b) { return a <= b ? a : b; }
inline unsigned int umin(unsigned int a, unsigned int b) { return a <= b ? a : b; }
inline unsigned long ulmin(unsigned long a, unsigned long b) { return a <= b ? a : b; }
inline char bmax(char a, char b) { return a >= b ? a : b; }
inline short smax(short a, short b) { return a >= b ? a : b; }
inline int max(int a, int b) { return a >= b ? a : b; }
inline long lmax(long a, long b) { return a >= b ? a : b; }
inline unsigned char ubmax(unsigned char a, unsigned char b) { return a >= b ? a : b; }
inline unsigned short usmax(unsigned short a, unsigned short b) { return a >= b ? a : b; }
inline unsigned int umax(unsigned int a, unsigned int b) { return a >= b ? a : b; }
inline unsigned long ulmax(unsigned long a, unsigned long b) { return a >= b ? a : b; }
inline float clampf(float val, float min, float max) { return (val < min) ? min : (val > max) ? max : val; }
inline double clamp(double val, double min, double max) { return (val < min) ? min : (val > max) ? max : val; }
inline char bclampi(char val, char min, char max) { return (val < min) ? min : (val > max) ? max : val; }
inline short sclampi(short val, short min, short max) { return (val < min) ? min : (val > max) ? max : val; }
inline int clampi(int val, int min, int max) { return (val < min) ? min : (val > max) ? max : val; }
inline long lclampi(long val, long min, long max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned char ubclampi(unsigned char val, unsigned char min, unsigned char max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned short usclampi(unsigned short val, unsigned short min, unsigned short max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned int uclampi(unsigned int val, unsigned int min, unsigned int max) { return (val < min) ? min : (val > max) ? max : val; }
inline unsigned long ulclampi(unsigned long val, unsigned long min, unsigned long max) { return (val < min) ? min : (val > max) ? max : val; }
inline float atan2pif(float x, float y) { return atan2(x,y)/M_PI; }
inline double atan2pi(double x, double y) { return atan2(x,y)/M_PI; }
inline float mixf(float x, float y, float a) { return x + (y-x)*a; }
inline double mix(double x, double y, double a) { return x + (y-x)*a; }
#endif

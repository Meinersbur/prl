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
#ifndef PENCIL_PROTOTYPES_H
#define PENCIL_PROTOTYPES_H
char babs(char x) __attribute__((const));
short sabs(short x) __attribute__((const));
int abs(int x) __attribute__((const));
long labs(long x) __attribute__((const));
unsigned char ubabs(unsigned char x) __attribute__((const));
unsigned short usabs(unsigned short x) __attribute__((const));
unsigned int uabs(unsigned int x) __attribute__((const));
unsigned long ulabs(unsigned long x) __attribute__((const));
char bmax(char a, char b) __attribute__((const));
short smax(short a, short b) __attribute__((const));
//int max(int a, int b) __attribute__((const));
long lmax(long a, long b) __attribute__((const));
unsigned char ubmax(unsigned char a, unsigned char b) __attribute__((const));
unsigned short usmax(unsigned short a, unsigned short b) __attribute__((const));
unsigned int umax(unsigned int a, unsigned int b) __attribute__((const));
unsigned long ulmax(unsigned long a, unsigned long b) __attribute__((const));
char bmin(char a, char b) __attribute__((const));
short smin(short a, short b) __attribute__((const));
int min(int a, int b) __attribute__((const));
long lmin(long a, long b) __attribute__((const));
unsigned char ubmin(unsigned char a, unsigned char b) __attribute__((const));
unsigned short usmin(unsigned short a, unsigned short b) __attribute__((const));
unsigned int umin(unsigned int a, unsigned int b) __attribute__((const));
unsigned long ulmin(unsigned long a, unsigned long b) __attribute__((const));
float clampf(float val, float min, float max) __attribute__((const));
double clamp(double val, double min, double max) __attribute__((const));
float degreesf(float rad) __attribute__((const));
double degrees(double rad) __attribute__((const));
float mixf(float x, float y, float a) __attribute__((const));
double mix(double x, double y, double a) __attribute__((const));
float radiansf(float deg) __attribute__((const));
double radians(double deg) __attribute__((const));
float signf(float x) __attribute__((const));
double sign(double x) __attribute__((const));
float stepf(float edge, float x) __attribute__((const));
double step(double edge, double x) __attribute__((const));
//float fmodf(float x) __attribute__((const));
//double fmod(double x) __attribute__((const));
float remainderf(float a, float b) __attribute__((const));
double remainder(double a, double b) __attribute__((const));
float remquof(float x, float y, int* quo) __attribute__((const));
double remquo(double x, double y, int* quo) __attribute__((const));
float fmaf(float a, float b, float c) __attribute__((const));
double fma(double a, double b, double c) __attribute__((const));
float madf(float a, float b, float c) __attribute__((const));
double mad(double a, double b, double c) __attribute__((const));
float fdimf(float x, float y) __attribute__((const));
double fdim(double x, double y) __attribute__((const));
float fmaxf(float a, float b) __attribute__((const));
double fmax(double a, double b) __attribute__((const));
float fminf(float a, float b) __attribute__((const));
double fmin(double a, double b) __attribute__((const));
float fabsf(float x) __attribute__((const));
double fabs(double x) __attribute__((const));
float expf(float x) __attribute__((const));
double exp(double x) __attribute__((const));
float exp2f(float x) __attribute__((const));
double exp2(double x) __attribute__((const));
float exp10f(float x) __attribute__((const));
double exp10(double x) __attribute__((const));
float expm1f(float x) __attribute__((const));
double expm1(double x) __attribute__((const));
float logf(float x) __attribute__((const));
double log(double x) __attribute__((const));
float log2f(float x) __attribute__((const));
double log2(double x) __attribute__((const));
float log10f(float x) __attribute__((const));
double log10(double x) __attribute__((const));
float log1pf(float x) __attribute__((const));
double log1p(double x) __attribute__((const));
float powf(float x, float y) __attribute__((const));
double pow(double x, double y) __attribute__((const));
float pownf(float x, int y) __attribute__((const));
double pown(double x, int y) __attribute__((const));
float powrf(float x, float y) __attribute__((const));
double powr(double x, double y) __attribute__((const));
float sqrtf(float x) __attribute__((const));
double sqrt(double x) __attribute__((const));
float rsqrtf(float x) __attribute__((const));
double rsqrt(double x) __attribute__((const));
float cbrtf(float x) __attribute__((const));
double cbrt(double x) __attribute__((const));
float rootnf(float x, int n) __attribute__((const));
double rootn(double x, int n) __attribute__((const));
float hypotf(float x, float y) __attribute__((const));
double hypot(double x, double y) __attribute__((const));
float sinf(float x) __attribute__((const));
double sin(double x) __attribute__((const));
float cosf(float x) __attribute__((const));
double cos(double x) __attribute__((const));
float tanf(float x) __attribute__((const));
double tan(double x) __attribute__((const));
float asinf(float x) __attribute__((const));
double asin(double x) __attribute__((const));
float acosf(float x) __attribute__((const));
double acos(double x) __attribute__((const));
float atanf(float x) __attribute__((const));
double atan(double x) __attribute__((const));
float atan2f(float x, float y) __attribute__((const));
double atan2(double x, double y) __attribute__((const));
float sinpif(float x) __attribute__((const));
double sinpi(double x) __attribute__((const));
float cospif(float x) __attribute__((const));
double cospi(double x) __attribute__((const));
float tanpif(float x) __attribute__((const));
double tanpi(double x) __attribute__((const));
float asinpif(float x) __attribute__((const));
double asinpi(double x) __attribute__((const));
float acospif(float x) __attribute__((const));
double acospi(double x) __attribute__((const));
float atanpif(float x) __attribute__((const));
double atanpi(double x) __attribute__((const));
float atan2pif(float x, float y) __attribute__((const));
double atan2pi(double x, double y) __attribute__((const));
float sinhf(float x) __attribute__((const));
double sinh(double x) __attribute__((const));
float coshf(float x) __attribute__((const));
double cosh(double x) __attribute__((const));
float tanhf(float x) __attribute__((const));
double tanh(double x) __attribute__((const));
float asinhf(float x) __attribute__((const));
double asinh(double x) __attribute__((const));
float acoshf(float x) __attribute__((const));
double acosh(double x) __attribute__((const));
float atanhf(float x) __attribute__((const));
double atanh(double x) __attribute__((const));
float erff(float x) __attribute__((const));
double erf(double x) __attribute__((const));
float erfcf(float x) __attribute__((const));
double erfc(double x) __attribute__((const));
float tgammaf(float x) __attribute__((const));
double tgamma(double x) __attribute__((const));
float lgammaf(float x) __attribute__((const));
double lgamma(double x) __attribute__((const));
float lgamma_rf(float x, int* signp) __attribute__((const));
double lgamma_r(double x, int* signp) __attribute__((const));
float ceilf(float x) __attribute__((const));
double ceil(double x) __attribute__((const));
float floorf(float x) __attribute__((const));
double floor(double x) __attribute__((const));
float truncf(float x) __attribute__((const));
double trunc(double x) __attribute__((const));
float roundf(float x) __attribute__((const));
double round(double x) __attribute__((const));
float logbf(float x) __attribute__((const));
double logb(double x) __attribute__((const));
float nextafterf(float from, float to) __attribute__((const));
double nextafter(double from, double to) __attribute__((const));
float copysignf(float x, float y) __attribute__((const));
double copysign(double x, double y) __attribute__((const));
char bclampi(char val, char min, char max) __attribute__((const));
short sclampi(short val, short min, short max) __attribute__((const));
int clampi(int val, int min, int max) __attribute__((const));
long lclampi(long val, long min, long max) __attribute__((const));
unsigned char ubclampi(unsigned char val, unsigned char min, unsigned char max) __attribute__((const));
unsigned short usclampi(unsigned short val, unsigned short min, unsigned short max) __attribute__((const));
unsigned int uclampi(unsigned int val, unsigned int min, unsigned int max) __attribute__((const));
unsigned long ulclampi(unsigned long val, unsigned long min, unsigned long max) __attribute__((const));
#endif

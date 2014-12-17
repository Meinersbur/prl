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
int iabs(int x) __attribute__((const));
long labs(long x) __attribute__((const));
unsigned char ubabs(unsigned char x) __attribute__((const));
unsigned short usabs(unsigned short x) __attribute__((const));
unsigned int uabs(unsigned int x) __attribute__((const));
unsigned long ulabs(unsigned long x) __attribute__((const));
char bmax(char a, char b) __attribute__((const));
short smax(short a, short b) __attribute__((const));
int imax(int a, int b) __attribute__((const));
long lmax(long a, long b) __attribute__((const));
unsigned char ubmax(unsigned char a, unsigned char b) __attribute__((const));
unsigned short usmax(unsigned short a, unsigned short b) __attribute__((const));
unsigned int umax(unsigned int a, unsigned int b) __attribute__((const));
unsigned long ulmax(unsigned long a, unsigned long b) __attribute__((const));
char bmin(char a, char b) __attribute__((const));
short smin(short a, short b) __attribute__((const));
int imin(int a, int b) __attribute__((const));
long lmin(long a, long b) __attribute__((const));
unsigned char ubmin(unsigned char a, unsigned char b) __attribute__((const));
unsigned short usmin(unsigned short a, unsigned short b) __attribute__((const));
unsigned int umin(unsigned int a, unsigned int b) __attribute__((const));
unsigned long ulmin(unsigned long a, unsigned long b) __attribute__((const));
float clampf(float val, float min, float max) __attribute__((const));
double clampd(double val, double min, double max) __attribute__((const));
float degreesf(float rad) __attribute__((const));
double degreesd(double rad) __attribute__((const));
float mixf(float x, float y, float a) __attribute__((const));
double mixd(double x, double y, double a) __attribute__((const));
float radiansf(float deg) __attribute__((const));
double radiansd(double deg) __attribute__((const));
float signf(float x) __attribute__((const));
double signd(double x) __attribute__((const));
float stepf(float edge, float x) __attribute__((const));
double stepd(double edge, double x) __attribute__((const));
float fmodf(float x) __attribute__((const));
double fmodd(double x) __attribute__((const));
float remainderf(float a, float b) __attribute__((const));
double remainderd(double a, double b) __attribute__((const));
float remquof(float x, float y, int* quo) __attribute__((const));
double remquod(double x, double y, int* quo) __attribute__((const));
float fmaf(float a, float b, float c) __attribute__((const));
double fmad(double a, double b, double c) __attribute__((const));
float madf(float a, float b, float c) __attribute__((const));
double madd(double a, double b, double c) __attribute__((const));
float fdimf(float x, float y) __attribute__((const));
double fdimd(double x, double y) __attribute__((const));
float fmaxf(float a, float b) __attribute__((const));
double fmaxd(double a, double b) __attribute__((const));
float fminf(float a, float b) __attribute__((const));
double fmind(double a, double b) __attribute__((const));
float fabsf(float x) __attribute__((const));
double fabsd(double x) __attribute__((const));
float expf(float x) __attribute__((const));
double expd(double x) __attribute__((const));
float exp2f(float x) __attribute__((const));
double exp2d(double x) __attribute__((const));
float exp10f(float x) __attribute__((const));
double exp10d(double x) __attribute__((const));
float expm1f(float x) __attribute__((const));
double expm1d(double x) __attribute__((const));
float logf(float x) __attribute__((const));
double logd(double x) __attribute__((const));
float log2f(float x) __attribute__((const));
double log2d(double x) __attribute__((const));
float log10f(float x) __attribute__((const));
double log10d(double x) __attribute__((const));
float log1pf(float x) __attribute__((const));
double log1pd(double x) __attribute__((const));
float powf(float x, float y) __attribute__((const));
double powd(double x, double y) __attribute__((const));
float pownf(float x, int y) __attribute__((const));
double pownd(double x, int y) __attribute__((const));
float powrf(float x, float y) __attribute__((const));
double powrd(double x, double y) __attribute__((const));
float sqrtf(float x) __attribute__((const));
double sqrtd(double x) __attribute__((const));
float rsqrtf(float x) __attribute__((const));
double rsqrtd(double x) __attribute__((const));
float cbrtf(float x) __attribute__((const));
double cbrtd(double x) __attribute__((const));
float rootnf(float x, int n) __attribute__((const));
double rootnd(double x, int n) __attribute__((const));
float hypotf(float x, float y) __attribute__((const));
double hypotd(double x, double y) __attribute__((const));
float sinf(float x) __attribute__((const));
double sind(double x) __attribute__((const));
float cosf(float x) __attribute__((const));
double cosd(double x) __attribute__((const));
float tanf(float x) __attribute__((const));
double tand(double x) __attribute__((const));
float asinf(float x) __attribute__((const));
double asind(double x) __attribute__((const));
float acosf(float x) __attribute__((const));
double acosd(double x) __attribute__((const));
float atanf(float x) __attribute__((const));
double atand(double x) __attribute__((const));
float atan2f(float x, float y) __attribute__((const));
double atan2d(double x, double y) __attribute__((const));
float sinpif(float x) __attribute__((const));
double sinpid(double x) __attribute__((const));
float cospif(float x) __attribute__((const));
double cospid(double x) __attribute__((const));
float tanpif(float x) __attribute__((const));
double tanpid(double x) __attribute__((const));
float asinpif(float x) __attribute__((const));
double asinpid(double x) __attribute__((const));
float acospif(float x) __attribute__((const));
double acospid(double x) __attribute__((const));
float atanpif(float x) __attribute__((const));
double atanpid(double x) __attribute__((const));
float atan2pif(float x, float y) __attribute__((const));
double atan2pid(double x, double y) __attribute__((const));
float sinhf(float x) __attribute__((const));
double sinhd(double x) __attribute__((const));
float coshf(float x) __attribute__((const));
double coshd(double x) __attribute__((const));
float tanhf(float x) __attribute__((const));
double tanhd(double x) __attribute__((const));
float asinhf(float x) __attribute__((const));
double asinhd(double x) __attribute__((const));
float acoshf(float x) __attribute__((const));
double acoshd(double x) __attribute__((const));
float atanhf(float x) __attribute__((const));
double atanhd(double x) __attribute__((const));
float erff(float x) __attribute__((const));
double erfd(double x) __attribute__((const));
float erfcf(float x) __attribute__((const));
double erfcd(double x) __attribute__((const));
float tgammaf(float x) __attribute__((const));
double tgammad(double x) __attribute__((const));
float lgammaf(float x) __attribute__((const));
double lgammad(double x) __attribute__((const));
float lgamma_rf(float x, int* signp) __attribute__((const));
double lgamma_rd(double x, int* signp) __attribute__((const));
float ceilf(float x) __attribute__((const));
double ceild(double x) __attribute__((const));
float floorf(float x) __attribute__((const));
double floord(double x) __attribute__((const));
float truncf(float x) __attribute__((const));
double truncd(double x) __attribute__((const));
float roundf(float x) __attribute__((const));
double roundd(double x) __attribute__((const));
float logbf(float x) __attribute__((const));
double logbd(double x) __attribute__((const));
float nextafterf(float from, float to) __attribute__((const));
double nextafterd(double from, double to) __attribute__((const));
float copysignf(float x, float y) __attribute__((const));
double copysignd(double x, double y) __attribute__((const));
char bclampi(char val, char min, char max) __attribute__((const));
short sclampi(short val, short min, short max) __attribute__((const));
int iclampi(int val, int min, int max) __attribute__((const));
long lclampi(long val, long min, long max) __attribute__((const));
unsigned char ubclampi(unsigned char val, unsigned char min, unsigned char max) __attribute__((const));
unsigned short usclampi(unsigned short val, unsigned short min, unsigned short max) __attribute__((const));
unsigned int uclampi(unsigned int val, unsigned int min, unsigned int max) __attribute__((const));
unsigned long ulclampi(unsigned long val, unsigned long min, unsigned long max) __attribute__((const));
#endif

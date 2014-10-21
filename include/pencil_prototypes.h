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
float fabsf(float x);
double fabs(double x);
char babs(char x);
short sabs(short x);
int abs(int x);
long labs(long x);
unsigned char ubabs(unsigned char x);
unsigned short usabs(unsigned short x);
unsigned int uabs(unsigned int x);
unsigned long ulabs(unsigned long x);
float fminf(float a, float b);
double fmin(double a, double b);
char bmin(char a, char b);
short smin(short a, short b);
int min(int a, int b);
long lmin(long a, long b);
unsigned char ubmin(unsigned char a, unsigned char b);
unsigned short usmin(unsigned short a, unsigned short b);
unsigned int umin(unsigned int a, unsigned int b);
unsigned long ulmin(unsigned long a, unsigned long b);
float fmaxf(float a, float b);
double fmax(double a, double b);
char bmax(char a, char b);
short smax(short a, short b);
int max(int a, int b);
long lmax(long a, long b);
unsigned char ubmax(unsigned char a, unsigned char b);
unsigned short usmax(unsigned short a, unsigned short b);
unsigned int umax(unsigned int a, unsigned int b);
unsigned long ulmax(unsigned long a, unsigned long b);
float clampf(float val, float min, float max);
double clamp(double val, double min, double max);
char bclampi(char val, char min, char max);
short sclampi(short val, short min, short max);
int clampi(int val, int min, int max);
long lclampi(long val, long min, long max);
unsigned char ubclampi(unsigned char val, unsigned char min, unsigned char max);
unsigned short usclampi(unsigned short val, unsigned short min, unsigned short max);
unsigned int uclampi(unsigned int val, unsigned int min, unsigned int max);
unsigned long ulclampi(unsigned long val, unsigned long min, unsigned long max);
float tanf(float x);
double tan(double x);
float atanf(float x);
double atan(double x);
float atan2f(float x, float y);
double atan2(double x, double y);
float atan2pif(float x, float y);
double atan2pi(double x, double y);
float hypotf(float x, float y);
double hypot(double x, double y);
float sqrtf(float x);
double sqrt(double x);
float expf(float x);
double exp(double x);
float powf(float x, float y);
double pow(double x, double y);
float ceilf(float x);
double ceil(double x);
float floorf(float x);
double floor(double x);
float mixf(float x, float y, float a);
double mix(double x, double y, double a);
#endif

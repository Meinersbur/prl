/*
 * Copyright (c) 2014, ARM Limited
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
 */

#ifndef PENCIL_COMPAT_H
#define PENCIL_COMPAT_H

#include <assert.h>
#include <stdbool.h>

/* PENCIL built-in functions prototypes and (possible) implementations. */
#include "pencil_lib.h"


/* PENCIL functions */
#define __pencil_kill(...)
#define __pencil_use(...)
#define __pencil_def(...)
#define __pencil_maybe() 1
#define __pencil_assume(...)
#define __pencil_assert(...) assert(__VA_ARGS__)

#define ACCESS(X)

/* Additional PENCIL types not in C99 */
/* half */
#if __ARM_FP16_ARGS
  /* use __fp16 only if usable as arguments (some older ARM targets only supports it in structs or arrays) */
  #define half __fp16
#else /* __ARM_FP16_ARGS */
  /* 16-bit floating-point is not supported: fallback to float */
  #define half float
#endif /* __ARM_FP16_ARGS */

#endif /* PENCIL_COMPAT_H */

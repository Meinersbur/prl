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

/* There are 2 ways to use PENCIL file:
 *
 * 1. As Normal C file:
 *    - All PENCIL specific functions (like __pencil_assert) must be ignored.
 *    - All PENCIL built-in functions (math functions, for example) must be
 *      implemented.
 *
 * 2. As PENCIL file supplied to the PENCIL compiler:
 *    - All PENCIL specific functions must be preserved.
 *    - All PENCIL build-in function must be declared (to make the front-end
 *      happy), but not implemented.
 */

#ifndef PENCIL_H
#define PENCIL_H

/* Preprocessor aliases for PENCIL builtins */
#define USE __pencil_use
#define DEF __pencil_def
#define MAYBE __pencil_maybe()

#ifdef __PENCIL__
/* The file is processed by the PENCIL-to-OpenCL code generator. */

/* Custom stdbool.h */
#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined 1

/* PENCIL built-in functions prototypes only. */
#include "pencil_prototypes.h"

/* PENCIL-specific macros */
#define ACCESS(...) __attribute__((pencil_access(__VA_ARGS__)))

#else /* __PENCIL__ */
/* The file is processed as a C file. */

/* PENCIL to C compatibility layer. */
#include "pencil_compat.h"

#endif /* __PENCIL__ */
#endif /* PENCIL_H */

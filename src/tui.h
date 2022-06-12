/* tui.h - v0.0 - public domain text user interface library - sellesoft 2022

   To use this libaray, do this in *one* C or C++ file:
      #define TUI_IMPLEMENTATION
	  #include "tui.h"

   Placed in the public domain and also MIT licensed.
   See end of file for detailed license information.

Table of Contents
-----------------------------
   @tui_api
   @tui_api_macros
   @tui_api_types
   @tui_api_funcs
   @tui_impl
   @tui_impl_include
   @tui_impl_macros
   @tui_impl_funcs
   @tui_license

Compile-Time Options
-----------------------------
   #define TUI_REALLOC(ptr,size) better_realloc
   #define TUI_FREE(ptr)         better_free

      These defines only need to be set in the file containing #define TUI_IMPLEMENTATION.
      By default, tui uses libc realloc() and free() for memory management.
      You must either define both, or neither.

Documentation
-----------------------------


Notes
-----------------------------


*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//@tui_api
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#pragma once
#ifndef TUI_API
#ifdef   __cplusplus
extern "C" {
#endif //__cplusplus


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_api_macros
#if      defined(TUI_REALLOC) && !defined(TUI_FREE) || !defined(TUI_REALLOC) && defined(TUI_FREE)
#  error "You must define both TUI_REALLOC and TUI_FREE, or neither."
#endif //defined(TUI_REALLOC) && !defined(TUI_FREE) || !defined(TUI_REALLOC) && defined(TUI_FREE)
#if      !defined(TUI_REALLOC) && !defined(TUI_FREE)
#  include <stdlib.h>
#  define TUI_REALLOC(ptr,size) realloc((ptr),(size))
#  define TUI_FREE(ptr)         free((ptr))
#endif //!defined(TUI_REALLOC) && !defined(TUI_FREE)


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_api_types
#if __STDC_VERSION__ >= 199901L //C99
#  include <stdint.h>
typedef int32_t      tui_s32;
typedef uint32_t     tui_u32;
typedef float        tui_f32;
typedef s32          tui_b32;
#else
typedef singed int   tui_s32;
typedef unsigned int tui_u32;
typedef float        tui_f32;
typedef s32          tui_b32;
#endif //__STDC_VERSION__ >= 199901L



//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_api_funcs



#ifdef   __cplusplus
}
#endif //__cplusplus
#endif //TUI_API
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//@tui_impl
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#ifdef TUI_IMPLEMENTATION


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_impl_includes
#ifndef STB_DS_IMPLEMENTATION
#  define STB_DS_IMPLEMENTATION
#endif
#include "stb_ds.h"


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_impl_macros
#define forI(a) for(s32 i=0; i < (a); ++i)
#define forX(x,a) for(s32 x=0; x < (a); ++x)
#define For(T,a) for(T* it = a; it != a+arrlen(a); ++it)
#define ForX(T,x,a) for(T* x = a; x != a+arrlen(a); ++x)
#define arrinit(a,n) a = 0; arrsetcap(a,n);


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_impl_funcs



#undef forI
#undef forX
#undef For
#undef ForX
#undef arrinit
#endif //TUI_IMPLEMENTATION
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//@tui_license
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/* This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2022 sellesoft
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
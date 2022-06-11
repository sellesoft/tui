#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

#define true 1
#define false 0
typedef int8_t    s8;
typedef int16_t   s16;
typedef int32_t   s32;
typedef int64_t   s64;
typedef ptrdiff_t spt;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef size_t    upt; 
typedef float     f32;
typedef double    f64;
typedef s32       b32;
typedef wchar_t   wchar;
typedef u32       Type;
typedef u32       Flags;

#define print(fmt, ...) wprintf(L##fmt, ##__VA_ARGS__)
#define fprint(file,fmt, ...) fwprintf(file, L##fmt, ##__VA_ARGS__); fflush(file)
#define println(fmt, ...) wprintf(L##fmt"\n", ##__VA_ARGS__)
#define fprintln(file,fmt, ...) fwprintf(file, L##fmt"\n", ##__VA_ARGS__); fflush(file)
#define forI(a) for(int i=0; i < (a); ++i)
#define forX(x,a) for(int x=0; x < (a); ++x)
#define For(a) for(typeof(*(a))* it = a; it != a+arrlen(a); ++it)
#define ForX(x,a) for(typeof(*(a))* x = a; x != a+arrlen(a); ++x)
#define arrinit(a,n) a = 0; arrsetcap(a,n);

#define CastFromMember(structName,memberName,ptr) ((structName*)((u8*)(ptr) - OffsetOfMember(structName,memberName)))

typedef struct str{
	wchar* data;
	u64    count;
} str;


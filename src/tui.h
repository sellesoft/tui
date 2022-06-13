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
   @tui_impl_vars
   @tui_impl_funcs
   @tui_license

Compile-Time Options
-----------------------------
   These defines only need to be set in the file containing #define TUI_IMPLEMENTATION.

   #define TUI_REALLOC(ptr,size) better_realloc
   #define TUI_FREE(ptr)         better_free
      By default, tui uses libc realloc() and free() for memory management.
      You must either define both, or neither.

   #define TUI_LOGGING
   #define TUI_LOGGING_PATH "path_to_log"
      By default, tui does not log errors or messages, but you can specify it to write them
	  to a log file by defining TUI_LOGGING. The default TUI_LOGGING_PATH is simply "tui_log.txt".

   #define TUI_ASSERT(condition, ...) better_assert
      By default, tui uses libc assert() for assertion. The ... in the macro isn't
	  necessary to be handled as it's only used for a error message at the code location.


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

#ifndef TUI_LOGGING_PATH
#  define TUI_LOGGING_PATH "log.txt"
#endif //TUI_LOGGING_PATH

#ifndef TUI_ASSERT
#  include <assert.h>
#  define TUI_ASSERT(condition,...) assert((condition))
#endif //TUI_ASSERT


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

enum{
    TUI_Item_Text,
	TUI_Item_Input,
};

typedef struct TUI_Cell{
	u32 cp;
	u32 bg;
	u32 fg;
} TUI_Cell;

typedef struct TUI_Item{
	u32 x,y ;//local pos in panel
	u32 sx, sy; //size, only used for some items like Input
	u32 type;
	str text;
} TUI_Item;

//panel to be drawn (ui window)
typedef struct TUI_Panel{
	u32 x0, x1, y0, y1;
	TUI_Item* items;	//stb_arr
	str title;
	f32 title_align; //how the title is aligned, valid values are 0 to 1, default 0 
} TUI_Panel;

//NOTE(delle): everything is done in cells, not pixels or floats
typedef struct TUI_Terminal{
	HWND out_pipe;
    HWND in_pipe;

	b32 quit;
	b32 dirty; //console window needs to be redrawn
	b32 ascii;
	
	u32* buffer;

	TUI_Cell* cells;
	u32 width, height;

	u32 cursor_x, cursor_y;

	u32 default_fg, default_bg;

	TUI_Panel* panels; //stb_arr
	
	u32 text_input[256];
	u32 text_input_count;
} TUI_Terminal;


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_api_funcs
TUI_Terminal* tui_init();

b32 tui_update();

void tui_cleanup();

void tui_hide_cursor();

void tui_show_cursor();


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
#  define STBDS_REALLOC(context,ptr,size) TUI_REALLOC(ptr,size)
#  define STBDS_FREE(context,ptr)         TUI_FREE(ptr)
#  define STBDS_ASSERT(x)                 TUI_ASSERT(x)
#  define STB_DS_IMPLEMENTATION
#  include "stb_ds.h"
#endif

#include <windows.h>


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_impl_macros
#define forI(a) for(s32 i=0; i < (a); ++i)
#define forX(x,a) for(s32 x=0; x < (a); ++x)
#define For(T,a) for(T* it = a; it != a+arrlen(a); ++it)
#define ForX(T,x,a) for(T* x = a; x != a+arrlen(a); ++x)
#define arrinit(a,n) a = 0; arrsetcap(a,n)

#ifdef TUI_LOGGING
#  define print(fmt, ...) wprintf(L##fmt, ##__VA_ARGS__)
#  define fprint(file,fmt, ...) fwprintf(file, L##fmt, ##__VA_ARGS__); fflush(file)
#  define println(fmt, ...) wprintf(L##fmt"\n", ##__VA_ARGS__)
#  define fprintln(file,fmt, ...) fwprintf(file, L##fmt"\n", ##__VA_ARGS__); fflush(file)
#  define Log(fmt, ...) fprintln(log_file, fmt, ##__VA_ARGS__)
#  define Win32Func(func,...) if(!func(##__VA_ARGS__)) tui_win32_error(L#func, true)
#else
#  define print(fmt, ...)
#  define fprint(file,fmt, ...)
#  define println(fmt, ...)
#  define fprintln(file,fmt, ...)
#  define Log(fmt, ...)
#  define Win32Func(func,...) func(##__VA_ARGS__)
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_impl_vars
const u32 TUI_BORDER_H  = 0x2500; //'─'
const u32 TUI_BORDER_V  = 0x2502; //'│'
const u32 TUI_BORDER_TL = 0x250C; //'┌'
const u32 TUI_BORDER_TR = 0x2510; //'┐'
const u32 TUI_BORDER_BL = 0x2514; //'└'
const u32 TUI_BORDER_BR = 0x2518; //'┘'
const u32 TUI_BORDER_H_ASCII  = (u32)'-';
const u32 TUI_BORDER_V_ASCII  = (u32)'|';
const u32 TUI_BORDER_TL_ASCII = (u32)'+';
const u32 TUI_BORDER_TR_ASCII = (u32)'+';
const u32 TUI_BORDER_BL_ASCII = (u32)'+';
const u32 TUI_BORDER_BR_ASCII = (u32)'+';
const wchar* TUI_ESCAPESEQ_COLOR        = L"\x1b[38;2;xxx;xxx;xxxm\x1b[48;2;xxx;xxx;xxxm";
const u32    TUI_ESCAPESEQ_COLOR_SIZE   = sizeof(L"\x1b[38;2;xxx;xxx;xxxm\x1b[48;2;xxx;xxx;xxxm");
const wchar* TUI_ESCAPESEQ_NEWLINE      = L"\n";//u"\x1b[1E\x1b[0G";
const u32    TUI_ESCAPESEQ_NEWLINE_SIZE = sizeof(L"\n");//sizeof(u"\x1b[1E\x1b[0G");

TUI_Terminal* tui_terminal;
DWORD tui_restore_console_mode;

#ifdef TUI_LOGGING
FILE* tui_log_file;
#endif


//-////////////////////////////////////////////////////////////////////////////////////////////////
//@tui_impl_funcs
void tui_win32_error(wchar* func_name, b32 cleanup){ //NOTE(delle) always return true so it can be in an if statement with Win32Func
#ifdef TUI_LOGGING
	LPWSTR msg_buffer;
	DWORD error = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
				   0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), &msg_buffer, 0, 0);
	Log("[win32] %ls failed with error %u : %ls", func_name, error, msg_buffer);
	fclose(log_file);
	LocalFree(msg_buffer);
#endif
	if(cleanup) tui_cleanup();
	ExitProcess(0);
}

u32 wchar_from_codepoint(wchar* out, u32 codepoint){
	u32 advance = 0;
	if(codepoint < 0x10000){
		advance = 1;
		if(out){
			out[0] = (wchar)codepoint;
		}
	}else if(codepoint <= 0x10FFFF){
		advance = 2;
		if(out){
			u64 v = codepoint - 0x10000;
			out[0] = 0xD800 + (v >> 10);
			out[1] = 0xDC00 + (v & 0x03FF);
		}
	}else{
		fprintln(log_file, "invalid codepoint: %u", codepoint);
	}
	return advance;
}

void tui_set_cell(u32 x, u32 y, u32 codepoint, u32 bg, u32 fg){
	TUI_ASSERT(tui_terminal != 0, "tui_init() must be called before any tui functions");
	TUI_ASSERT(x < terminal->width);
	TUI_ASSERT(y < terminal->height);

	u32 cell_index = (terminal->width * y) + x;
	tui_terminal->cells[cell_index].cp = codepoint;
	tui_terminal->cells[cell_index].bg = bg;
	tui_terminal->cells[cell_index].fg = fg;
}

TUI_Terminal* tui_init(){
	//setup unicode support on Win32
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdin),  _O_U16TEXT);

#ifdef TUI_LOGGING
	tui_log_file = fopen(TUI_LOGGING_PATH, "w+");
	Log("[TUI] init()");
#endif

	//allocate the terminal
	tui_terminal = TUI_REALLOC(0, sizeof(Terminal));
	tui_terminal->ascii      = 0;
	tui_terminal->default_fg = 0xffffffff;
	tui_terminal->default_bg = 0x00000000;
	tui_terminal->dirty      = 0;
	arrinit(tui_terminal->panels, 4);

	//get the stdin and stdout handles
	terminal->out_pipe = GetStdHandle(STD_OUTPUT_HANDLE);
	terminal->in_pipe  = GetStdHandle(STD_INPUT_HANDLE);
	if(terminal->out_pipe == INVALID_HANDLE_VALUE) tui_win32_error(L"GetStdHandle(STD_OUTPUT_HANDLE)");
    if(terminal->in_pipe  == INVALID_HANDLE_VALUE) tui_win32_error(L"GetStdHandle(STD_INPUT_HANDLE)");

	//store the current console mode for restoring on exit
	//NOTE(delle) not using Win32Func here since restore might not be valid so we don't want to cleanup
	if(!GetConsoleMode(terminal->in_pipe, &tui_restore_console_mode)) tui_win32_error(L"GetConsoleMode", false);

	//set the new console mode
	Win32Func(SetConsoleMode, terminal->in_pipe,
			  ENABLE_MOUSE_INPUT|ENABLE_VIRTUAL_TERMINAL_INPUT|ENABLE_VIRTUAL_TERMINAL_PROCESSING|ENABLE_WINDOW_INPUT);

	//get the initial terminal size and clear the terminal
	CONSOLE_SCREEN_BUFFER_INFO csbi; GetConsoleScreenBufferInfo(terminal->out_pipe, &csbi);
	tui_resize_terminal(csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
	tui_clear_terminal();
}

b32 tui_update(){
	Log("%u, %u", terminal->cursor_x, terminal->cursor_y);

	INPUT_RECORD records[5] = {0};
	u32 ninputs = 0;
	if(!GetNumberOfConsoleInputEvents(terminal->in_pipe, &ninputs)){
		printlasterr(L"ReadConsoleInput");
		SetConsoleMode(terminal->in_pipe, restore_console_mode);
		return 0;
	}
	u32 nread = 0;
	if(!ReadConsoleInput(terminal->in_pipe, records, 5, &nread)){
		printlasterr(L"ReadConsoleInput");
		SetConsoleMode(terminal->in_pipe, restore_console_mode);
		return 0;
	}

	forI(nread){
		INPUT_RECORD ir = records[i];
		switch(ir.EventType){
			case FOCUS_EVENT:{/*internal so ignore*/}break;
			case KEY_EVENT:{
				KEY_EVENT_RECORD rec = ir.Event.KeyEvent;
				switch(rec.wVirtualKeyCode){
					case 'Q':{
						terminal->quit = 1;
					}break;
					case VK_LEFT:{
						if(terminal->cursor_x) terminal->cursor_x--;
						COORD coord = {terminal->cursor_x, terminal->cursor_y};
						Win32Func(SetConsoleCursorPosition, terminal->out_pipe, coord);
					}break;
					case VK_UP:{
						if(terminal->cursor_y) terminal->cursor_y--;
						COORD coord = {terminal->cursor_x, terminal->cursor_y};
						Win32Func(SetConsoleCursorPosition, terminal->out_pipe, coord);
					}break;
					case VK_RIGHT:{
						if(terminal->cursor_x < terminal->width) terminal->cursor_x++;
						COORD coord = {terminal->cursor_x, terminal->cursor_y};
						Win32Func(SetConsoleCursorPosition, terminal->out_pipe, coord);
					}break;
					case VK_DOWN:{
						if(terminal->cursor_y < terminal->height) terminal->cursor_y++;
						COORD coord = {terminal->cursor_x, terminal->cursor_y};
						Win32Func(SetConsoleCursorPosition, terminal->out_pipe, coord);
					}break;
				}
			}break;
			case MENU_EVENT:{
				
			}break;
			case MOUSE_EVENT:{
				
				
			}break;
			case WINDOW_BUFFER_SIZE_EVENT:{
				WINDOW_BUFFER_SIZE_RECORD rec = ir.Event.WindowBufferSizeEvent;
				resize_terminal(rec.dwSize.X, rec.dwSize.Y);
				terminal->dirty = 1;
			}break;
		}
	}

	if(terminal->dirty){
		terminal->dirty = 0;
		clear_terminal();
		draw_terminal();
	}

	return !tui_terminal->quit;
}

void tui_cleanup(){
#ifdef TUI_LOGGING
	fclose(log_file);
#endif
	clear_terminal();
	show_cursor();
	SetConsoleMode(terminal->in_pipe, tui_restore_console_mode);
}

void tui_hide_cursor(){
	TUI_ASSERT(tui_terminal != 0, "tui_init() must be called before any tui functions");

	CONSOLE_CURSOR_INFO cci;
	Win32Func(GetConsoleCursorInfo, terminal->out_pipe, &cci);
	cci.bVisible = FALSE;
	Win32Func(SetConsoleCursorInfo, terminal->out_pipe, &cci);
}

void tui_show_cursor(){
	TUI_ASSERT(tui_terminal != 0, "tui_init() must be called before any tui functions");

	CONSOLE_CURSOR_INFO cci;
	Win32Func(GetConsoleCursorInfo, terminal->out_pipe, &cci);
	cci.bVisible = TRUE;
	Win32Func(SetConsoleCursorInfo, terminal->out_pipe, &cci);
}

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
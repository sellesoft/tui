/*Index:

*/


#include "common.h"
#include <windows.h>


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @types
enum{
    Item_Text,
	Item_Input,
};

typedef struct Cell{
	u32 cp;
	u32 bg;
	u32 fg;
} Cell;


typedef struct Item{
	u32 x,y ;//local pos in panel
	u32 sx, sy; //size, only used for some items like Input
	u32 type;
	str text;
}Item;

//panel to be drawn (ui window)
typedef struct Panel{
	u32 x0, x1, y0, y1;
	Item* items;	//stb_arr

} Panel;

Item* panel_add_item(Panel* p, u32 x, u32 y, u32 sx, u32 sy, u32 type){
	Item item;
	item.x = x;
	item.y = y;
	item.sx = sx;
	item.sy = sy;
	item.type = type;
	arrput(p->items, item);
}

//NOTE(delle): everything is done in cells, not pixels or floats
typedef struct Terminal{
	HWND out_pipe;
    HWND in_pipe;

	b32 quit;
	b32 dirty; //console window needs to be redrawn
	b32 ascii;

	Cell* cells;
	u32 width, height;

	u32 cursor_x, cursor_y;

	u32 default_fg, default_bg;

	Panel* panels; //stb_arr
	
	u32 text_input[256];
	u32 text_input_count;
} Terminal;


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @vars
PCWSTR CLEAR_CONSOLE = L"\x1b[2J";
u32 BORDER_H  = 0x2500; //'─'
u32 BORDER_V  = 0x2502; //'│'
u32 BORDER_TL = 0x250C; //'┌'
u32 BORDER_TR = 0x2510; //'┐'
u32 BORDER_BL = 0x2514; //'└'
u32 BORDER_BR = 0x2518; //'┘'
u32 BORDER_H_ASCII  = (u32)'-';
u32 BORDER_V_ASCII  = (u32)'|';
u32 BORDER_TL_ASCII = (u32)'+';
u32 BORDER_TR_ASCII = (u32)'+';
u32 BORDER_BL_ASCII = (u32)'+';
u32 BORDER_BR_ASCII = (u32)'+';

Terminal* terminal;
FILE* log_file;

#define Log(fmt, ...) //fprintln(log_file, fmt, ##__VA_ARGS__);

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @funcs
void printlasterr(const char* func_name){
    LPVOID msg_buffer;
	DWORD error = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
					 0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPWSTR)&msg_buffer, 0, 0);
	fprint(log_file,"[win32] %s failed with error %u : %ls", func_name, error, (LPWSTR)msg_buffer);
	fclose(log_file);
	LocalFree(msg_buffer);
	assert(false);
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

void set_cell(u32 x, u32 y, u32 codepoint, u32 bg, u32 fg){
	if(terminal->ascii){
		Log("set_cell x: %2u y: %2u cp: '%c'", x, y, (char)codepoint);
	}else{
		wchar w[3] = {0}; wchar_from_codepoint(w, codepoint);
		Log("set_cell x: %2u y: %2u cp: '%ls'", x, y, w);
	}
	assert(x < terminal->width); assert(y < terminal->height);
	u32 cell_index = (terminal->width * y) + x;
	terminal->cells[cell_index].cp = codepoint;
	terminal->cells[cell_index].bg = bg;
	terminal->cells[cell_index].fg = fg;
}

void clear_terminal(){
	Log("clear_terminal");
	if(!WriteFile(terminal->out_pipe, CLEAR_CONSOLE, (DWORD)(wcslen(CLEAR_CONSOLE)*sizeof(wchar)), 0, 0)){
		printlasterr("WriteFile");
	}
}

void draw_terminal(){
	Log("draw_terminal");

	if(terminal->panels){
		Log("npanels: %i", arrlen(terminal->panels));
		ForX(panel, terminal->panels){
			Log("panel: x0: %u, y0: %u, x1: %u, y1: %u", panel->x0, panel->y0, panel->x1, panel->y1);
			//draw panel outline
			set_cell(panel->x0, panel->y0, (terminal->ascii) ? BORDER_TL_ASCII : BORDER_TL, terminal->default_bg, terminal->default_fg);
			set_cell(panel->x1, panel->y0, (terminal->ascii) ? BORDER_TR_ASCII : BORDER_TR, terminal->default_bg, terminal->default_fg);
			set_cell(panel->x0, panel->y1, (terminal->ascii) ? BORDER_BL_ASCII : BORDER_BL, terminal->default_bg, terminal->default_fg);
			set_cell(panel->x1, panel->y1, (terminal->ascii) ? BORDER_BR_ASCII : BORDER_BR, terminal->default_bg, terminal->default_fg);
			for(u32 i = panel->x0+1; i < panel->x1; i++){
				set_cell(i, panel->y0, (terminal->ascii) ? BORDER_H_ASCII : BORDER_H, terminal->default_bg, terminal->default_fg);
				set_cell(i, panel->y1, (terminal->ascii) ? BORDER_H_ASCII : BORDER_H, terminal->default_bg, terminal->default_fg);
			}
			for(u32 i = panel->y0+1; i < panel->y1; i++){
				set_cell(panel->x0, i, (terminal->ascii) ? BORDER_V_ASCII : BORDER_V, terminal->default_bg, terminal->default_fg);
				set_cell(panel->x1, i, (terminal->ascii) ? BORDER_V_ASCII : BORDER_V, terminal->default_bg, terminal->default_fg);
			}

			ForX(item, panel->items){
				switch(item->type){
					case Item_Text:{
						forI(item->text.count){
							if(item->x+i==panel->x1)break;
							set_cell(panel->x0+1+item->x+i, panel->y0+1+item->y, item->text.data[i], terminal->default_bg, terminal->default_fg);
						}
					}break;
				}
			}

		}
	}
	
	//set_cell(0,0,'A',terminal->default_bg, terminal->default_fg);
	//set_cell(5,5,'B',terminal->default_bg, terminal->default_fg);
	//set_cell(10,10,'C',terminal->default_bg, terminal->default_fg);
	//set_cell(10,5,'D',terminal->default_bg, terminal->default_fg);
	//set_cell(5,10,'E',terminal->default_bg, terminal->default_fg);
	
	Log("There are %u cells", terminal->width*terminal->height);
	forI(terminal->width*terminal->height){
		COORD coord = {i%terminal->width, i/terminal->width};
		SetConsoleCursorPosition(terminal->out_pipe, coord);
		if(terminal->cells[i].cp == 0){
			if(!WriteFile(terminal->out_pipe, " ", 1, 0, 0)){
				printlasterr("WriteFile");
				return;
			}
		}else{
			if(terminal->ascii){
				Log("Writing '%c' to position x:%i, y:%i", terminal->cells[i].cp, coord.X, coord.Y);
				if(!WriteFile(terminal->out_pipe, &terminal->cells[i].cp, 1, 0, 0)){
					printlasterr("WriteFile");
					return;
				}
			}else{
				//convert u32 to u16
				wchar wcp[2];
				u32 advance = wchar_from_codepoint(wcp, terminal->cells[i].cp);
				if(!WriteFile(terminal->out_pipe, wcp, advance*sizeof(wchar), 0, 0)){
					printlasterr("WriteFile");
					return;
				}
			}
		}
	}

	COORD coord = {terminal->cursor_x, terminal->cursor_y};
	SetConsoleCursorPosition(terminal->out_pipe, coord);
}

void resize_terminal(u32 new_width, u32 new_height){
	Log("resize_terminal new size: %ux%u", new_width, new_height);
	Cell* new_cells = calloc(new_width*new_height, sizeof(Cell));

	//readjust positioning of panels
	
	terminal->dirty  = true;
	terminal->width  = new_width;
    terminal->height = new_height;
	terminal->cells  = new_cells;
}

int main(int argc, char** argv){
	//setup unicode support on Win32
	_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdin),  _O_U16TEXT);


	//// init ////
	log_file = fopen("log.txt", "w+");
	Log("init");

	terminal = calloc(1, sizeof(Terminal));
	terminal->ascii = 1;
	terminal->default_fg = 0xffffffff;
	terminal->default_bg = 0x00000000;
	terminal->dirty      = false;
	arrinit(terminal->panels, 4);

    terminal->out_pipe = GetStdHandle(STD_OUTPUT_HANDLE);
    if(terminal->out_pipe == INVALID_HANDLE_VALUE){
        printlasterr("GetStdHandle(STD_OUTPUT_HANDLE)");
        return 0;
    }
    terminal->in_pipe = GetStdHandle(STD_INPUT_HANDLE);
    if(terminal->in_pipe == INVALID_HANDLE_VALUE){
        printlasterr("GetStdHandle(STD_INPUT_HANDLE)");
        return 0;
    }

	DWORD restore_console_mode;
	if(!GetConsoleMode(terminal->in_pipe, &restore_console_mode)){
		printlasterr("GetConsoleMode");
		return 0;
	}

	SetConsoleMode(terminal->in_pipe, ENABLE_MOUSE_INPUT);
    //SetConsoleMode(terminal->in_pipe, ENABLE_VIRTUAL_TERMINAL_INPUT);
    SetConsoleMode(terminal->in_pipe, ENABLE_WINDOW_INPUT);
    
	CONSOLE_SCREEN_BUFFER_INFO csbi; GetConsoleScreenBufferInfo(terminal->out_pipe, &csbi);
	resize_terminal(csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);

	Panel panel;
	panel.x0 = 0;
    panel.x1 = terminal->width - 1;
    panel.y0 = 0;
    panel.y1 = terminal->height - 1;
	arrput(terminal->panels, panel);
	arrinit(terminal->panels[0].items, 10);
	Item* item = panel_add_item(terminal->panels,0,0,0,0,Item_Text);
	str s; 
	s.data = U"hello";
	s.count = 5;
	item->text = s;

	clear_terminal();
	draw_terminal();
	Log("w:%uh:%u", terminal->width, terminal->height);
	
	//// update ////
	Log("update");
	while(!terminal->quit){
		Log("---------");
		item->x = 20*(sin(time(0))+1)/2;
		terminal->dirty = 1;
		INPUT_RECORD records[5] = {0};
		u32 nread;
		if(!ReadConsoleInput(terminal->in_pipe, records, 5, &nread)){
			printlasterr("ReadConsoleInput");
			SetConsoleMode(terminal->in_pipe, restore_console_mode);
			return 0;
		}

		forI(nread){
			INPUT_RECORD ir = records[i];
			switch(ir.EventType){
				case FOCUS_EVENT:{/*internal so ignore*/}break;
				case KEY_EVENT:{
					KEY_EVENT_RECORD rec = ir.Event.KeyEvent;
					//char c = MapVirtualKeyW(rec.wVirtualKeyCode, MAPVK_VK_TO_CHAR);
					//fprintln(log_file,"%u", rec.wVirtualKeyCode);
					
					switch(rec.wVirtualKeyCode){
						case 'Q':{
							terminal->quit = true;
						}break;
						case VK_LEFT:{
							if(terminal->cursor_x) terminal->cursor_x--;
							COORD coord = {terminal->cursor_x, terminal->cursor_y};
							SetConsoleCursorPosition(terminal->out_pipe, coord);
						}break;
						case VK_UP:{
							if(terminal->cursor_y) terminal->cursor_y--;
							COORD coord = {terminal->cursor_x, terminal->cursor_y};
							SetConsoleCursorPosition(terminal->out_pipe, coord);
						}break;
						case VK_RIGHT:{
							if(terminal->cursor_x < terminal->width) terminal->cursor_x++;
							COORD coord = {terminal->cursor_x, terminal->cursor_y};
							SetConsoleCursorPosition(terminal->out_pipe, coord);
						}break;
						case VK_DOWN:{
							if(terminal->cursor_y < terminal->height) terminal->cursor_y++;
							COORD coord = {terminal->cursor_x, terminal->cursor_y};
							SetConsoleCursorPosition(terminal->out_pipe, coord);
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
				}break;
			}
		}

		if(terminal->dirty){
			terminal->dirty = false;
			clear_terminal();
			draw_terminal();
		}
	}
	arrlen(terminal->panels);
	fclose(log_file);
	SetConsoleMode(terminal->in_pipe, restore_console_mode);
	return 0;
}
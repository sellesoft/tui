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
	str title;
	f32 title_align; //how the title is aligned, valid values are 0 to 1, default 0 
} Panel;


//NOTE(delle): everything is done in cells, not pixels or floats
typedef struct Terminal{
	HWND out_pipe;
    HWND in_pipe;

	b32 quit;
	b32 dirty; //console window needs to be redrawn
	b32 ascii;
	
	u32* buffer;

	Cell* cells;
	u32 width, height;

	u32 cursor_x, cursor_y;

	u32 default_fg, default_bg;

	Panel* panels; //stb_arr
	
	u32 text_input[256];
	u32 text_input_count;
} Terminal;

const u16* termcol_escseq = L"\x1b[38;2;xxx;xxx;xxxm\x1b[48;2;xxx;xxx;xxxm";
const u32 termcol_escseq_size = sizeof(L"\x1b[38;2;xxx;xxx;xxxm\x1b[48;2;xxx;xxx;xxxm");
const u16* termnl_escseq = L"\n";//u"\x1b[1E\x1b[0G";
const u32 termnl_escseq_size = sizeof(L"\n");//sizeof(u"\x1b[1E\x1b[0G");


//-////////////////////////////////////////////////////////////////////////////////////////////////
// @vars
PCWSTR CLEAR_CONSOLE = L"\x1b[2J\x1b[3J";
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

#define Log(fmt, ...) fprintln(log_file, fmt, ##__VA_ARGS__)

//-////////////////////////////////////////////////////////////////////////////////////////////////
// @funcs
void printlasterr(wchar* func_name){
    LPVOID msg_buffer;
	DWORD error = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
					 0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (LPWSTR)&msg_buffer, 0, 0);
	Log("[win32] %ls failed with error %u : %ls", func_name, error, (LPWSTR)msg_buffer);
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

Panel* make_panel(u32 x0, u32 x1, u32 y0, u32 y1, str title){
	Panel panel;
	panel.x0 = x0;
    panel.x1 = x1;
    panel.y0 = y0;
    panel.y1 = y1;
	panel.title = title;
	panel.title_align = 0;
	arrput(terminal->panels, panel);
	arrinit(terminal->panels[0].items, 2);
	return terminal->panels + arrlen(terminal->panels) - 1;
}

Item* panel_add_item(Panel* p, u32 x, u32 y, u32 sx, u32 sy, u32 type){
	Item item;
	item.x = x;
	item.y = y;
	item.sx = sx;
	item.sy = sy;
	item.type = type;
	arrput(p->items, item);
	return p->items + arrlen(p->items) - 1;
}

void set_cell(u32 x, u32 y, u32 codepoint, u32 bg, u32 fg){
	assert(x < terminal->width); assert(y < terminal->height);
	if(terminal->ascii){
		//Log("set_cell() x: %2u y: %2u cp: '%c'", x, y, (char)codepoint);
	}else{
		wchar w[3] = {0}; wchar_from_codepoint(w, codepoint);
		//Log("set_cell() x: %2u y: %2u cp: '%ls'", x, y, w);
	}
	u32 cell_index = (terminal->width * y) + x;
	terminal->cells[cell_index].cp = codepoint;
	terminal->cells[cell_index].bg = bg;
	terminal->cells[cell_index].fg = fg;
}

void hide_cursor(){
	CONSOLE_CURSOR_INFO cci;
	if(!GetConsoleCursorInfo(terminal->out_pipe, &cci)){
		printlasterr(L"GetConsoleCursorInfo");
		return;
	}
	cci.bVisible = FALSE;
	if(!SetConsoleCursorInfo(terminal->out_pipe, &cci)){
		printlasterr(L"SetConsoleCursorInfo");
		return;
	}
}

void show_cursor(){
	CONSOLE_CURSOR_INFO cci;
	if(!GetConsoleCursorInfo(terminal->out_pipe, &cci)){
		printlasterr(L"GetConsoleCursorInfo");
		return;
	}
	cci.bVisible = TRUE;
	if(!SetConsoleCursorInfo(terminal->out_pipe, &cci)){
		printlasterr(L"SetConsoleCursorInfo");
		return;
	}
}

void clear_terminal(){
	Log("clear_terminal()");
	COORD coords = {0};
	DWORD written_chars;
	DWORD terminal_size = terminal->width*terminal->height;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(!GetConsoleScreenBufferInfo(terminal->out_pipe, &csbi)){
		printlasterr(L"GetConsoleScreenBufferInfo");
		return;
	}
	if(!FillConsoleOutputCharacterW(terminal->out_pipe, L' ', terminal_size, coords, &written_chars)){
		printlasterr(L"FillConsoleOutputCharacterW");
		return;
	}
	if(!GetConsoleScreenBufferInfo(terminal->out_pipe, &csbi)){
		printlasterr(L"GetConsoleScreenBufferInfo");
		return;
	}
	if(!FillConsoleOutputAttribute(terminal->out_pipe, csbi.wAttributes, terminal_size, coords, &written_chars)){
		printlasterr(L"FillConsoleOutputAttribute");
		return;
	}
	if(!SetConsoleCursorPosition(terminal->out_pipe, coords)){
		printlasterr(L"SetConsoleCursorPosition");
		return;
	}
	memset(terminal->cells, 0, terminal_size*sizeof(Cell));
}

//must dealloc yourself
wchar* cell_string(Cell cell, b32 applycol){
	wchar* str = (wchar*)malloc(sizeof(wchar)+termcol_escseq_size+20);
	memset(str, 0, sizeof(wchar)+termcol_escseq_size+20);
	wchar* cur = str;
	if(applycol){
		memcpy(str, termcol_escseq, termcol_escseq_size);
		wchar temp[50] = {0};
		cur+=7;
		//write foreground
		u32 cw = swprintf(temp, 50, 
			L"%03u;%03u;%03u",
			(cell.fg&0xff000000)>>24,
			(cell.fg&0x00ff0000)>>16,
			(cell.fg&0x0000ff00)>> 8
		);
		memcpy(cur, temp, cw*sizeof(wchar));
		//write background
		cur+=19;
		cw = swprintf(temp, 50, 
			L"%03u;%03u;%03u",
			(cell.bg&0xff000000)>>24,
			(cell.bg&0x00ff0000)>>16,
			(cell.bg&0x0000ff00)>> 8
		);
		memcpy(cur, temp, cw*sizeof(wchar));
		cur+=12;
	}
	//write character
	wchar wcp[2]={0};
	u32 advance = wchar_from_codepoint(wcp, cell.cp);
	memcpy(cur, wcp, sizeof(wchar));
	cur+=1;
	return str;
}

void draw_section(u32 x0, u32 y0, u32 x1, u32 y1){
	u32 dims = (x1-x0) * (y1-y0);
	
	u32 lastfg = 0;
	u32 lastbg = 0;
	COORD coord = {x0,y0};
	forI(dims){
		Cell c = terminal->cells[terminal->width * coord.Y + coord.X];
		wchar* s; 
		b32 nc = 0;
		if(lastfg!=c.fg||lastbg!=c.bg){
			lastfg = c.fg; lastbg = c.bg;
			s = cell_string(c, 1);
		} else s = cell_string(c, 0);

		SetConsoleCursorPosition(terminal->out_pipe, coord);
		if(!WriteConsoleW(terminal->out_pipe, s,1,0,0)){
			printlasterr(L"WriteConsoleW");
			return;
		}
		free(s);
		if(coord.X==x1) {coord.X=x0; coord.Y++;}
		else coord.X++;
	}
}

void draw_terminal(){
	Log("draw_terminal()");
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
			if(panel->title.data){
				u32 panelw = (panel->x1) - (panel->x0+1);
				u32 tstart = panel->x0+1 + (u32)ceil(panel->title_align * (panelw-panel->title.count));
				forI(panel->title.count){
					if(tstart+i==panel->x1) break;
					set_cell(tstart+i, panel->y0, panel->title.data[i], terminal->default_bg, terminal->default_fg);
				}
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
			//draw_section(panel->x0,panel->y0,panel->x1,panel->y1);
		}
	}
	COORD coord = {0,0};
	SetConsoleCursorPosition(terminal->out_pipe, coord);
	//malloc enough space for each cell to have a fg and bg color esc seq and a newline escseq at the end of each line
	u32 outsize = (termcol_escseq_size+sizeof(wchar))*(terminal->height*terminal->width)+termnl_escseq_size*(terminal->height-1);
	wchar* out = (wchar*)malloc(outsize);
	memset(out, 0, outsize);
	wchar* cur = out;
	wchar* linestart = cur;
	u32 lastbg = 0;
	u32 lastfg = 0;
	forI(terminal->width*terminal->height){
		wchar* chardbg = cur;
		Cell cell = terminal->cells[i];
		if(i%terminal->width == terminal->cursor_x && 
		   i/terminal->width == terminal->cursor_y){
			u32 save = cell.fg;
			cell.fg = cell.bg;
			cell.bg = save;	   
		}
		if(lastbg != cell.bg || lastfg != cell.fg){
			lastbg = cell.bg;
			lastfg = cell.fg;
			memcpy(cur, termcol_escseq, termcol_escseq_size);
			wchar temp[50] = {0};
			//"\x1b[38;2;xxx;xxx;xxxm\x1b[48;2;xxx;xxx;xxxm";
			cur+=7;
			//write foreground
			u32 cw = swprintf(temp, 50, 
				L"%03u;%03u;%03u",
				(terminal->cells[i].fg&0xff000000)>>24,
				(terminal->cells[i].fg&0x00ff0000)>>16,
				(terminal->cells[i].fg&0x0000ff00)>> 8
			);
			memcpy(cur, temp, cw*sizeof(wchar));
			//write background
			cur+=19;
			cw = swprintf(temp, 50, 
				L"%03u;%03u;%03u",
				(terminal->cells[i].bg&0xff000000)>>24,
				(terminal->cells[i].bg&0x00ff0000)>>16,
				(terminal->cells[i].bg&0x0000ff00)>> 8
			);
			memcpy(cur, temp, cw*sizeof(wchar));
			cur+=12;
		}
		
		//write character
		if(terminal->cells[i].cp == 0){
			terminal->cells[i].cp = (u32)' ';
		}
		wchar wcp[2]={0};
		u32 advance = wchar_from_codepoint(wcp, terminal->cells[i].cp);
		memcpy(cur, wcp, sizeof(wchar));
		cur+=1;
		//set newline if we are at the end of a line
		if(i&&(i+1%terminal->width == 0)){
			memcpy(cur, L"\n", sizeof(L"\n"));
			cur+=1;
		}
	}

	if(!WriteConsoleW(terminal->out_pipe, out, terminal->width*terminal->height,0,0)){
		printlasterr(L"WriteConsoleW");
		return;
	}

	 free(out);
	// coord = {terminal->cursor_x, terminal->cursor_y};
	// SetConsoleCursorPosition(terminal->out_pipe, coord);
}

void resize_terminal(u32 new_width, u32 new_height){
	Log("resize_terminal() new size: %ux%u", new_width, new_height);
	Cell* new_cells = calloc(new_width*new_height, sizeof(Cell));

	//readjust positioning of panels
	
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
	setvbuf(log_file,0,_IONBF,0);
	Log("init");

	terminal = calloc(1, sizeof(Terminal));
	terminal->ascii = false;
	terminal->default_fg = 0xffffffff;
	terminal->default_bg = 0x00000000;
	terminal->dirty      = false;
	arrinit(terminal->panels, 4);

    terminal->out_pipe = GetStdHandle(STD_OUTPUT_HANDLE);
    if(terminal->out_pipe == INVALID_HANDLE_VALUE){
        printlasterr(L"GetStdHandle(STD_OUTPUT_HANDLE)");
        return 0;
    }
    terminal->in_pipe = GetStdHandle(STD_INPUT_HANDLE);
    if(terminal->in_pipe == INVALID_HANDLE_VALUE){
        printlasterr(L"GetStdHandle(STD_INPUT_HANDLE)");
        return 0;
    }

	DWORD restore_console_mode;
	if(!GetConsoleMode(terminal->in_pipe, &restore_console_mode)){
		printlasterr(L"GetConsoleMode");
		return 0;
	}

	SetConsoleMode(terminal->in_pipe, ENABLE_MOUSE_INPUT);
    SetConsoleMode(terminal->in_pipe, ENABLE_VIRTUAL_TERMINAL_INPUT);
    //SetConsoleMode(terminal->out_pipe, ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleMode(terminal->in_pipe, ENABLE_WINDOW_INPUT);
    
	CONSOLE_SCREEN_BUFFER_INFO csbi; GetConsoleScreenBufferInfo(terminal->out_pipe, &csbi);
	resize_terminal(csbi.srWindow.Right - csbi.srWindow.Left + 1, csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
	str title = {U"tuilib",6};
	Panel* p0 = make_panel(0,10,0,10,title);
	p0->title_align = 0.5;
	p0->x1 = terminal->width-1;
	Item* i0 = panel_add_item(p0, 10, 5, 0,0,Item_Text);
	str text = {U"here is some text", 17};
	i0->text = text;
	clear_terminal();
	draw_terminal();
	hide_cursor();

	//// update ////
	Log("update");
	while(!terminal->quit){
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
					//char c = MapVirtualKeyW(rec.wVirtualKeyCode, MAPVK_VK_TO_CHAR);
					//fprintln(log_file,"%u", rec.wVirtualKeyCode);
					terminal->dirty = 1;
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
					terminal->dirty = true;
				}break;
			}
		}

		if(terminal->dirty){
			terminal->dirty = false;
			clear_terminal();
			draw_terminal();
		}
	}

	fclose(log_file);
	clear_terminal();
	show_cursor();
	SetConsoleMode(terminal->in_pipe, restore_console_mode);
	return 0;
}
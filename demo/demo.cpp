/* vim: set noexpandtab fileencoding=utf-8 nomodified nowrap textwidth=270 foldmethod=marker foldmarker={{{,}}} foldcolumn=4 ruler showcmd lcs=tab\:|- list: */
// ,g = gcc, exactly one space after "set"


#include "bios.h"
#include "Arduino.h"
#include "MemoryFree.h"
#include "charset_had.h"
extern uint8_t  __data_load_end;
extern uint8_t  __DATA_REGION_LENGTH__;
#define SleepFrames 2*60*50
uint16_t TimeToSleep=SleepFrames;
//
//////////////////////////// {{{ TETRIS
char state, x, y, waiting;
int score, high = 0;
char shape[10];       // holding the current shape (with offset)
const char tiles[7][10] PROGMEM = { { 0, 0, 1, 0, 0, -1, 1, -1,   0, 1 },     // Quadrat (with SRS offset)
                                    { -1, -1, 0, -1, 0, 0, 1, 0,  0, 0 },     // Z
                                    { -1, 0, 0, 0, 0, -1, 1, -1,  0, 0 },     // neg. Z
                                    { -1, 0, 0, 0, 1, 0, 2, 0,    0, 1 },     // I
                                    { -1, 0, 0, 0, 1, 0, 0, -1,   0, 0 },     // Podest
                                    { -1, -1, -1, 0, 0, 0, 1, 0,  0, 0 },     // L
                                    { -1, 0, 0, 0, 1, 0, 1, -1,   0, 0 }  };  // neg. L

void tetris_drawintro() { // {{{
	bios.clear();
	bios << YX( 10,  6) << COLOR( VGA_CYAN) << F("A R D U I N O   B L O C K S");
	bios << YX( 23,  13) << COLOR( VGA_YELLOW) << F("Press  SPACE");
}	// }}}
void drawfield() {	// {{{
	bios.clear();
	for (byte i=0; i<5; i++) bios << YX( i, 12) << COLOR( VGA_CYAN) << F("<!..........!>");
	for (byte i=5; i<20; i++) bios << YX(i,  12) << COLOR( VGA_WHITE) << F("<!..........!>");
	bios << YX( 20,  12) << COLOR( VGA_CYAN) << F("<!==========!>");
	bios << YX( 21,  12) << COLOR( VGA_MAGENTA) << F("  VVVVVVVVVV  ");
	bios << YX( 0, 0) << COLOR( VGA_GREEN) << F("SCORE 0");
	bios << YX( 0, 27) << F("HIGH");
	bios << YX( 0, 32);
	bios.printNum(high);
	bios << YX( 3, 0) << COLOR( VGA_CYAN) << F("CONTROLS");
	bios << YX( 5, 0) << COLOR( VGA_GRAY) << F("A - Left");
	bios << YX( 6, 0) << COLOR( VGA_GRAY) << F("D - Right");
	bios << YX( 7, 0) << COLOR( VGA_GRAY) << F("W - Rotate");
	bios << YX( 8, 0) << COLOR( VGA_GRAY) << F("S - Drop");
}	// }}}
boolean testShape(char x, char y) {	// {{{
	boolean isok = true;
	for (uint8_t i=0; i<8; i+=2)
	{
		if (x + shape[i] < 14+0 || x + shape[i] > 14+9) { isok = false; break; }	 // left / right 
		if (y + shape[i+1] < 0 || y + shape[i+1] > 19) { isok = false; break; }		 // top / bottom
		if (bios.vram[y + shape[i+1]][x + shape[i]] != '.') { isok = false; break; }	 // other item
	}
	return isok;
}	// }}}
void drawShape(char c) {	// {{{
	for (uint8_t i=0; i<8; i+=2) bios.vram[y + shape[i+1]][x + shape[i]] = c;
}	// }}}
void newShape() {				// {{{
	byte m = random(7);
	for (uint8_t i=0; i<10; i++) shape[i] = pgm_read_byte_near(&tiles[m][i]);
	x = 14+4; y = 1;
	if (testShape(x, y)) drawShape('#');
	else
	{
		bios << YX( 10,  14) << COLOR( VGA_RED) << F("GAME  OVER");
		bios << YX( 20,  13) << COLOR( VGA_YELLOW) << F("Press  SPACE");
		state = 2;
	}
}	// }}}
void rotShape() {	// {{{
	char rotshape[10];
	for(uint8_t i=0; i<10; i+=2)
	{
		rotshape[i] = shape[i+1];
		rotshape[i+1] = -shape[i];
	}
	for (uint8_t i=0; i<10; i++) shape[i] = rotshape[i];
	x += shape[8]; y+= shape[9];	// add rotated offset correction
}	// }}}
void updateField() {	// {{{
	uint8_t cleared = 0;		// count the number of rows cleared
	for (uint8_t y=0; y<20; y++)	// from top down
	{
		boolean rowfull = true;
		for(uint8_t x=14; x<14+10; x++) if (bios.vram[y][x] == '.') rowfull = false;		// is full?
		if (rowfull)
		{
			cleared++;
			for(uint8_t i=y; i>0; i--) 
				for(uint8_t x=14; x<14+10; x++) bios.vram[i][x] = bios.vram[i-1][x];	// copy all above
			for(uint8_t x=14; x<14+10; x++) bios.vram[0][x] = '.';				// free top line
		}
	}
	switch (cleared)
	{
		case 1: score += 40; break;
		case 2: score += 100; break;
		case 3: score += 200; break;
		case 4: score += 500; break;
	}
}	// }}}
void tetris_setup() {		// {{{
bios.inverting=false;
	tetris_drawintro();
	state = 0;
}	// }}}
void tetris_loop() {		// {{{
	uint16_t frame = bios.frames;
	byte key = bios.get_key();
if (key==xF12){ if(bios.current_output==BIOS_VGA) { bios.set_output(BIOS_RCA); } else { bios.set_output(BIOS_VGA);} ; };
	switch(state)
	{
		case 0:
			if (key == ' ')
			{
				randomSeed(bios.frames); random();
				drawfield();
				newShape();
				waiting = 40; bios.frames = 0; score = 0; state = 1;
			}
			break;
		case 1:
			if (key != 0)
			{
				if (key == 's' || key == x2) { waiting -= 40; score++; }
				else {
					drawShape('.');				 // clear the old shape  
					switch (key) {
						case 'a':
						case x4:
							if (testShape(x-1, y)) x--;
							break;
						case 'd':
						case x6:
							if (testShape(x+1, y)) x++;
							break;
						case 'w':
						case x8:
							rotShape(); rotShape(); rotShape();		// 3x ccw = cw
							if (testShape(x, y) == false)			// if cannot fit
							{
								if (testShape(x-1, y)) x--;		// test left
								else if (testShape(x+1, y)) x++;	// test right
								else if (testShape(x-2, y)) x-=2;	// 
								else if (testShape(x+2, y)) x+=2;
								else { rotShape(); }			// put it back 
							}
							break;
					};
					drawShape('#');				 // Draw the shape
				}
			}
			if (waiting-- < 0)					// FALLEN
			{
				waiting += (40 - (bios.frames >> 9));
				drawShape('.');
				if (testShape(x, y+1)) { y++; drawShape('#'); }
				else
				{
					drawShape('#');			// fix shape
					updateField();			// clear full rows
					bios.set_cursor(0, 6);bios.printNum(score); 
					if (score > high) { high = score; bios.set_cursor(0,32);bios.printNum(high); }
					newShape();			// pick a new shape if possible
					drawShape('#');
				}
			}  
			break;
		case 2:
//			if (key == ' ') { tetris_drawintro(); state = 0; }
			 if (score == high) 
					bios << YX( 0, 13) << F("YOU ARE BEST");
				while(' '!=bios.get_key()){};
				state=3;
			break;
	};
	while (frame == bios.frames);
}	// }}}
///////////////////////// }}}
////////////////////////// {{{ Matrix
#define MAXLINES 100
struct line { uint8_t x, y, h; };
line lines[MAXLINES];

void Matrix_setup() {
	for (byte i=0; i<MAXLINES; i++) {
		lines[i].x = random(1,BIOS_COLS-1);		// make a random line
		lines[i].y = random(1,BIOS_ROWS-1);
		lines[i].h = random(3, 15);
	};
	for(byte i=0; i<BIOS_ROWS; i++) bios.cram[i] = 0b0100 | random(2);
}
void Matrix_loop() {
	for (byte i=0; i<MAXLINES; i++) {
		lines[i].y++;
		if (lines[i].y >= 1 && lines[i].y < BIOS_ROWS-1) bios.vram[lines[i].y][lines[i].x] = random(33, 127);
		if (lines[i].y - lines[i].h >= 1) {
			if (lines[i].y - lines[i].h < BIOS_ROWS-1) {
				bios.vram[lines[i].y - lines[i].h][lines[i].x] = 32;
			} else {
				lines[i].x = random(1,BIOS_COLS-1);		// make a random line
				lines[i].y = 0;
				lines[i].h = random(3, 15);
			};
		};
	};
	bios.wait(5);
}
///////////////////////// }}}
////////////////////////// {{{ Had
enum dirs {right=0,up,left,down};
class CItem {
	public:
		uint8_t x,y;
		dirs h;
		const char *tiles;
		CItem(uint8_t y, uint8_t x, dirs h, const char *t) : x(x),y(y),h(h),tiles(t) {};
		void show() { bios.vram[y][x]= pgm_read_byte_near(&tiles[h]); };
		void hide(char c) { bios.vram[y][x]=c; };
};
/*
const char tiles_head[] PROGMEM = ">^<v";
const char tiles_tail[] PROGMEM = "~!-;";
const char tiles_body[4][4] PROGMEM = { 	// telo from -> to (dirs) (telo[l->l] je ocas [l] vse je jinak, predchozi smer->novy smer, smer->opacny je ocas
	// l   u    r    d     << TO
	{'a', 'b', '~', 'd'},	// l - FROM
	{'e', 'f', 'g', '!'},	// u
	{'-', 'j', 'k', 'l'},	// r
	{'m', ';', 'o', 'p'}	// d
};
const char tiles_grass[] PROGMEM = ".";
const char tiles_wall[] PROGMEM = "#";
const char tiles_fruit[] PROGMEM = "*";
*/
const char tiles_head[] PROGMEM = ">^<v";
const char tiles_tail[] PROGMEM = "~!=;";
const unsigned char tiles_body[4][4] PROGMEM = { 	// telo from -> to (dirs) (telo[l->l] je ocas [l] vse je jinak, predchozi smer->novy smer, smer->opacny je ocas
	//    r      u      l      d     << TO
	{   0x89 , 0x8D , '='  , 0x82  },	// r
	{   0x80 , 0x7C , 0x8b ,  '!'  },	// u
	{   '~'  , 0x86 , '-'  , 0x8A  },	// l - FROM
	{   0x8C ,  ';' , 0x88 ,  'l'  }	// d
};
const char tiles_grass[] PROGMEM = ".";
const char tiles_wall[] PROGMEM = "\xA0";
const char tiles_fruit[] PROGMEM = "\xA1\xA2\xA3";
// {{{ isXYZ
#define isIT(kind) bool is_##kind(char c) { \
	for (uint8_t i=0; i< sizeof(tiles_##kind)-1;++i) if (uint8_t(c)==pgm_read_byte_near(&tiles_##kind[i]) )return true;\
	return false;\
}
isIT(grass)
isIT(wall)
isIT(fruit)
bool is_body(char c) {
	for (uint8_t y=0; y<4;++y) for (uint8_t x=0; x<4;++x) if (uint8_t(c)==pgm_read_byte_near(&tiles_body[y][x])) return true;
	return false;
};
// }}}
char move(uint8_t &y, uint8_t &x, dirs h) {	// {{{ posune ve smeru a vrati co tam je
	switch (h) {
		case dirs::right:	x++; break;
		case dirs::up   :	y--; break;
		case dirs::left :	x--; break;
		case dirs::down :	y++; break;
	};
	return bios.vram[y][x];
}	// }}}
dirs get_dir(char c) {	// {{{
//	for (dirs y=dirs::right; y<=dirs::down;++y) for (dirs x=dirs::right; x<=dirs::down;++x) if (c==tiles_body[y][x]) return x;
	for (uint8_t y=0; y<4;++y) for (uint8_t x=0; x<4;++x) if (uint8_t(c)==pgm_read_byte_near(&tiles_body[y][x])) return dirs(x);
	return dirs::right;
}	// }}}
void random_place(char c) {	// {{{
	uint8_t x,y;
	do {
	x = random(1,BIOS_COLS-2);
	y = random(2,BIOS_ROWS-2);
	} while ( not is_grass(bios.vram[y][x]));
	bios.vram[y][x]=c;
}	// }}}
uint16_t had_high=0;
void Had(){
	uint16_t count=0;
	bios.clear(pgm_read_byte_near(&tiles_grass[0]));
	for(uint8_t x=0; x<BIOS_COLS; ++x) {
		bios.vram[0][x]=' ';
		bios.vram[1][x]=pgm_read_byte_near(&tiles_wall[random(sizeof(tiles_wall)-1)]);
		bios.vram[BIOS_ROWS-1][x]=pgm_read_byte_near(&tiles_wall[random(sizeof(tiles_wall)-1)]);
	};
	for(uint8_t y=2; y<BIOS_ROWS-1; ++y) {
		bios.vram[y][0]=pgm_read_byte_near(&tiles_wall[random(sizeof(tiles_wall)-1)]);
		bios.vram[y][BIOS_COLS-1]=pgm_read_byte_near(&tiles_wall[random(sizeof(tiles_wall)-1)]);
	};
	CItem head(BIOS_ROWS/2,BIOS_COLS/2+1,dirs::right,tiles_head);
//	bios.vram[BIOS_ROWS/2][BIOS_COLS/2]=tiles_body[dirs::right][dirs::right]; // mini telo, at 
	CItem tail(BIOS_ROWS/2,BIOS_COLS/2,dirs::right,tiles_tail);
	head.show();
	tail.show();
	random_place(tiles_fruit[0]);
	bios.get_key();
	uint8_t pass=0;
	while(true){
		if (++pass>20) {pass=0;random_place(pgm_read_byte_near(&tiles_fruit[random(sizeof(tiles_fruit)-1)]));};
		bios.wait(score>20*8?1:20-(score>>3));
		uint16_t k=bios.get_key();
		dirs lh=head.h;
		switch (k) {
			case x6: case '6': case 'd': case 'D': head.h=dirs::right; break;
			case x8: case '8': case 'w': case 'W': head.h=dirs::up; break;
			case x4: case '4': case 'a': case 'A': head.h=dirs::left; break;
			case x2: case '2': case 's': case 'S': head.h=dirs::down; break;
			case xF12:{ if(bios.current_output==BIOS_VGA) { bios.set_output(BIOS_RCA); } else { bios.set_output(BIOS_VGA);} ; }; break;
			default: break;
		};
		uint8_t x=head.x;
		uint8_t y=head.y;
		char c=move(y,x,head.h);
		if (is_wall(c))      {  bios << YX(1,3) << COLOR(VGA_RED) << F(" * * * H L A V O U   N E * * * "); break;}
		else if (is_body(c)) {  bios << YX(1,3) << COLOR(VGA_RED) << F(" * * *   J A U V A J S   * * * "); break;}
		else if (is_fruit(c)) {  head.hide(pgm_read_byte_near(&tiles_body[lh][head.h])); head.x=x;head.y=y;head.show(); count++;}
		else if (is_grass(c)) {  head.hide(pgm_read_byte_near(&tiles_body[lh][head.h])); head.x=x;head.y=y;head.show(); 
					tail.hide(pgm_read_byte_near(&tiles_grass[random(sizeof(tiles_grass)-1)])); tail.h=get_dir(move(tail.y, tail.x, tail.h)); tail.show();}
		else {};
		if (had_high <count) { had_high=count; bios << YX(0,0) << COLOR(VGA_GREEN); };
		bios << YX(0,0) << F("SCORE: ") << count << YX(0,20) << F("HIGH: ") << had_high;
		bios << YX(0,30) << bios.frames << ' ' ;
	};
	bios << YX(BIOS_ROWS-1,2) << COLOR(VGA_GREEN) << F(" * *  P R E S S   S P A C E  * * ");
	while (' ' !=bios.get_key()){};
}
////////////////////////////// }}}
void AsciiTable() {	// {{{
	bios.clear();

	for(uint8_t y=0;y<16;y++) { bios.vram[7+y][0] = bios.hexa_digits[y]; bios.vram[6][1+y] = bios.hexa_digits[y]; };
	for(uint8_t y=0;y<16;y++)
		for(uint8_t x=0;x<16;x++)
			bios.vram[7+y][1+x] = 16*y+x;
	bios << YX(BIOS_ROWS-1,0) << COLOR(VGA_GREEN) << F(" * *  P R E S S   S P A C E  * * ");
	uint16_t c;
	do {
		c=bios.get_key();
		if (c) {
			bios.inverting=true;
			bios.invert();
			bios.inverting=false;
			switch (c) {
				case xF12:{ if(bios.current_output==BIOS_VGA) { bios.set_output(BIOS_RCA); } else { bios.set_output(BIOS_VGA);} ; }; break;
				case x4: if (bios.cursor.x) bios.cursor.x--; break;
				case x6: if (bios.cursor.x<BIOS_COLS-1) bios.cursor.x++  ; break;
				case x8: if (bios.cursor.y) bios.cursor.y--; break;
				case x2: if (bios.cursor.y<BIOS_ROWS-1) bios.cursor.y++; break;
				case xDot: case '.': if (bios.chardef == &charset) bios.chardef=&charset_had; else bios.chardef = &charset; break;
				};
			bios.inverting=true;
			bios.invert();
			if ((bios.cursor.x >= 1) and (bios.cursor.x < 1+16) and (bios.cursor.y >= 7) and (bios.cursor.y < 7+16)) {
				bios.vram[6][18] = bios.hexa_digits[bios.cursor.y-7];
				bios.vram[6][19] = bios.hexa_digits[bios.cursor.x-1];
			} else { 
				bios.vram[6][18] = ' ';
				bios.vram[6][19] = ' ';
			};
		};
	} while (' ' != c);
}	// }}}
void logo() {	// {{{
//	bios.inverting=true;
	bios << YX(1,2) << COLOR(VGA_YELLOW) << F(" **** Nano Home Computer 2.7 **** ");
	bios << YX(2,2) << COLOR(VGA_GREEN) << F("Free: RAM: ") << freeMemory() << F(" B; FLASH: ") << HEX_MAXIMUM_SIZE - (uintptr_t)&__data_load_end << F(" B");
	bios << YX(3,2) << COLOR(VGA_CYAN) << F("Used: FLASH ") <<  (uintptr_t)&__data_load_end << F(" / ") << HEX_MAXIMUM_SIZE << F(" B");
	bios << YX(4,2) << F("Screen size: ") <<COLOR(VGA_GREEN) << ' ' << BIOS_COLS << 'x' << BIOS_ROWS;
	bios << YX(5,2) << F("Compiled: " _DATE_ " " __TIME__ " \r\n?");
}	// }}}
void setup() {	// {{{
	bios.set_output(BIOS_RCA);      // Inicializace BIOS
	bios.set_output(BIOS_VGA);              // Inicializace BIOS
	bios.set_output(BIOS_RCA);      // Inicializace BIOS
	bios.set_output(BIOS_VGA);              // Inicializace BIOS
	//	bios.set_output();
	bios.clear(' ',VGA_WHITE);
	logo();
	bios.inverting=true;
	bios.invert();
	bios.inverting=false;
}	// }}}
// {{{ JUNK
//bios.set_output(BIOS_RCA);      // Inicializace BIOS
//	bios.set_output(BIOS_VGA);              // Inicializace BIOS
//	bios.set_color(VGA_GREEN); 	// Nastavení barvy na zelenou
//	bios.print("Hello\r\n\t World");
//	bios.inverting=true;

/*
	//
//	bios.scroll();			// cursor not scrolled
//	bios.clear(' '+0x80,VGA_YELLOW);	// cursor to 0,0
	//
	int line=4;
	bios.set_cursor(line++, 10);    	// Nastavení kurzoru na (0, 0)
	bios.set_color(VGA_GREEN); 	// Nastavení barvy na zelenou
	bios.write('A');          	// Zapsání znaku 'A' na obrazovku
	//
//	bios.set_output(BIOS_RCA);      // Inicializace BIOS
	bios.set_cursor(line++, 5);    	// Nastavení kurzoru na (0, 0)
	bios.set_color(VGA_GREEN); 	// Nastavení barvy na zelenou
	bios.write('B');          	// Zapsání znaku 'A' na obrazovku
//	bios.set_output(BIOS_VGA);
	bios.invert();			// inverts character at actual cursor
	// print interprets \r\n\t chars, write doesnot
	bios.print('c');
	bios.print("Hello\r\n\t World");
	bios.print(F("from the \r\n\tFLASH"));
	bios.printNum(1234, 18,'#');
	bios.printHex(0x9876, 6);
	bios.printBin(0b011101101, 2);
	//
	bios << YX(line++, 1) << COLOR(VGA_DKRED) << "Write:";
	bios.write('C');
	bios.write("Hello\r\n\t World");
	bios.writeNum(1234);
	bios.writeHex(0x9867,8,'$');
	bios.writeBin(234);
	//
	bios.set_cursor(line++,6);
	bios.get_cursorY();
	bios.get_cursorX();
	//
	bios.set_rowcolor(line-3, VGA_RED);
	bios.set_color(VGA_GREEN);	// current row
	//
	bios.get_key();
	bios.get_scancode();
	bios.printBin(bios.ps2status,16,'#');
	while(1){};
}}} */

extern "C" {extern void VRAMtest();};
extern "C" {extern void VRAMtest2();};
uint16_t c,lc;
void loop(){
	c=bios.get_key();
	if (c) {
		bios.inverting=true;
		bios.invert();
		bios.inverting=false;
		switch (c) {
			case xEsc: bios.clear(lc); break;
			case xF1:  { for (long i=0;i<10000;++i) bios.vram[((i/BIOS_COLS)%BIOS_ROWS)][i%BIOS_COLS]=uint8_t(i&0xFF); logo();}; break;
			case xF2: { for (int i=0; i<1000;++i)  VRAMtest(); logo();} break;
			case xF3: { for (int i=0; i<1000;++i)  VRAMtest2(); logo();} break;
			case xF4: { tetris_setup(); do { tetris_loop();} while (state!=3); bios.clear(' ',VGA_WHITE);logo();}; break;
			case xF5: { Matrix_setup(); do { Matrix_loop();} while (!bios.get_key()); bios.clear(' ',VGA_WHITE);logo();}; break;
			case xF6: { Had(); bios.clear(' ',VGA_WHITE);logo();}; break;
			case xF7: { AsciiTable(); bios.clear(' ',VGA_WHITE);logo();}; break;
			case xF12:{ if(bios.current_output==BIOS_VGA) { bios.set_output(BIOS_RCA); } else { bios.set_output(BIOS_VGA);} ; }; break;
			case x4: if (bios.cursor.x) bios.cursor.x--; break;
			case x6: if (bios.cursor.x<BIOS_COLS-1) bios.cursor.x++  ; break;
			case x8: if (bios.cursor.y) bios.cursor.y--; break;
			case x2: if (bios.cursor.y<BIOS_ROWS-1) bios.cursor.y++; break;
			case '\n': bios << '\r'; // fall thru
			default: bios << char(c & 0xFF); lc=c; break;
		};
		bios.inverting=true;
		bios.invert();
		TimeToSleep=SleepFrames;
	};
	
	
	bios.inverting=false;
	bios.wait(1);
	YX cc=bios.cursor; bios << YX(25,23) << F("frame ") << bios.frames << ' ' << cc;
	if (! --TimeToSleep) { Matrix_setup(); do { Matrix_loop();} while (!bios.get_key()); bios.clear(' ',VGA_WHITE);logo();TimeToSleep=SleepFrames;}; 



};
int main(){ setup(); while (true) {loop();}; };

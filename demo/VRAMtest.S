; VRAMtest - fill VideoRAM with sequences of bytes (00..FF and repeat). Save last char for next start, so each pass is different.
; VRAMtest2 - fill VideoRAM with the same byte and increase it for next run.

#include "bios.h"


.section .bss
fillchar: .byte 0

.section .text
.global VRAMtest
VRAMtest:
	ldi XL,lo8(vram)		;		;# 1 cycles, 2 bytes
	ldi XH,hi8(vram)		;		;# 1 cycles, 2 bytes
	ldi r24,lo8(BIOS_COLS*BIOS_ROWS);		;# 1 cycles, 2 bytes
	ldi r25,hi8(BIOS_COLS*BIOS_ROWS+256)		;# 1 cycles, 2 bytes
	lds r23,fillchar		;		;# 2 cycles, 4 bytes
loop:
	st X+,r23			;		;# 2 cycles, 2 bytes
	inc r23				;		;# 1 cycles, 2 bytes
	dec r24				;		;# 1 cycles, 2 bytes
	brne loop			;		;# 1-2 cycles, 2 bytes
	dec r25				;		;# 1 cycles, 2 bytes
	brne loop			;		;# 1-2 cycles, 2 bytes
	sts fillchar,r23		;		;# 2 cycles, 4 bytes
	ret				;		;# 4 cycles, 2 bytes

.global VRAMtest2
VRAMtest2:
	ldi XL,lo8(vram)		;		;# 1 cycles, 2 bytes
	ldi XH,hi8(vram)		;		;# 1 cycles, 2 bytes
	ldi r24,lo8(BIOS_COLS*BIOS_ROWS);		;# 1 cycles, 2 bytes
	ldi r25,hi8(BIOS_COLS*BIOS_ROWS+256)		;# 1 cycles, 2 bytes
	lds r23,fillchar		;		;# 2 cycles, 4 bytes
loop2:
	st X+,r23			;		;# 2 cycles, 2 bytes
	dec r24				;		;# 1 cycles, 2 bytes
	brne loop2			;		;# 1-2 cycles, 2 bytes
	dec r25				;		;# 1 cycles, 2 bytes
	brne loop2			;		;# 1-2 cycles, 2 bytes
	inc r23				;		;# 1 cycles, 2 bytes
	sts fillchar,r23		;		;# 2 cycles, 4 bytes
	ret				;		;# 4 cycles, 2 bytes

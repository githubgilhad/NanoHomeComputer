#ifndef CHARSETHAD_H
#define CHARSETHAD_H

	#ifdef __cplusplus
		#include <inttypes.h>
		#include <avr/pgmspace.h>
		extern	 const uint8_t charset_had[9][256] PROGMEM;	// VGA just 8 top lines, RCA all 9
	#else
		.extern charset_had
	#endif

#endif

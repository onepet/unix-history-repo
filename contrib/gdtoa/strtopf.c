/****************************************************************

The author of this software is David M. Gay.

Copyright (C) 1998, 2000 by Lucent Technologies
All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appear in all
copies and that both that the copyright notice and this
permission notice and warranty disclaimer appear in supporting
documentation, and that the name of Lucent or any of its entities
not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

LUCENT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
IN NO EVENT SHALL LUCENT OR ANY OF ITS ENTITIES BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

****************************************************************/

/* Please send bug reports to
	David M. Gay
	Bell Laboratories, Room 2C-463
	600 Mountain Avenue
	Murray Hill, NJ 07974-0636
	U.S.A.
	dmg@bell-labs.com
 */

#include "gdtoaimp.h"

 int
#ifdef KR_headers
strtopf(s, sp, f) CONST char *s; char **sp; float *f;
#else
strtopf(CONST char *s, char **sp, float *f)
#endif
{
#ifdef Sudden_Underflow
	static FPI fpi = { 24, 1-127-24+1,  254-127-24+1, 1, 1 };
#else
	static FPI fpi = { 24, 1-127-24+1,  254-127-24+1, 1, 0 };
#endif
	ULong bits[1], *L;
	Long exp;
	int k;

	k = strtodg(s, sp, &fpi, &exp, bits);
	L = (ULong*)f;
	switch(k & STRTOG_Retmask) {
	  case STRTOG_NoNumber:
	  case STRTOG_Zero:
		L[0] = 0;
		break;

	  case STRTOG_Normal:
	  case STRTOG_NaNbits:
		L[0] = bits[0] & 0x7fffff | exp + 0x7f + 23 << 23;
		break;

	  case STRTOG_Denormal:
		L[0] = bits[0];
		break;

	  case STRTOG_Infinite:
		L[0] = 0x7f800000;
		break;

	  case STRTOG_NaN:
		L[0] = 0x7fffffff;
	  }
	if (k & STRTOG_Neg)
		L[0] |= 0x80000000L;
	return k;
	}

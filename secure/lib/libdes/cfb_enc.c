/* lib/des/cfb_enc.c */
/* Copyright (C) 1995 Eric Young (eay@mincom.oz.au)
 * All rights reserved.
 * 
 * This file is part of an SSL implementation written
 * by Eric Young (eay@mincom.oz.au).
 * The implementation was written so as to conform with Netscapes SSL
 * specification.  This library and applications are
 * FREE FOR COMMERCIAL AND NON-COMMERCIAL USE
 * as long as the following conditions are aheared to.
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.  If this code is used in a product,
 * Eric Young should be given attribution as the author of the parts used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Eric Young (eay@mincom.oz.au)
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include "des_locl.h"

/* The input and output are loaded in multiples of 8 bits.
 * What this means is that if you hame numbits=12 and length=2
 * the first 12 bits will be retrieved from the first byte and half
 * the second.  The second 12 bits will come from the 3rd and half the 4th
 * byte.
 */
void des_cfb_encrypt(in, out, numbits, length, schedule, ivec, encrypt)
unsigned char *in;
unsigned char *out;
int numbits;
long length;
des_key_schedule schedule;
des_cblock (*ivec);
int encrypt;
	{
	register unsigned long d0,d1,v0,v1,n=(numbits+7)/8;
	register unsigned long mask0,mask1;
	register unsigned long l=length;
	register int num=numbits;
	unsigned long ti[2];
	unsigned char *iv;

	if (num > 64) return;
	if (num > 32)
		{
		mask0=0xffffffffL;
		if (num == 64)
			mask1=mask0;
		else	mask1=(1L<<(num-32))-1;
		}
	else
		{
		if (num == 32)
			mask0=0xffffffffL;
		else	mask0=(1L<<num)-1;
		mask1=0x00000000;
		}

	iv=(unsigned char *)ivec;
	c2l(iv,v0);
	c2l(iv,v1);
	if (encrypt)
		{
		while (l >= n)
			{
			l-=n;
			ti[0]=v0;
			ti[1]=v1;
			des_encrypt((unsigned long *)ti,schedule,DES_ENCRYPT);
			c2ln(in,d0,d1,n);
			in+=n;
			d0=(d0^ti[0])&mask0;
			d1=(d1^ti[1])&mask1;
			l2cn(d0,d1,out,n);
			out+=n;
			/* 30-08-94 - eay - changed because l>>32 and
			 * l<<32 are bad under gcc :-( */
			if (num == 32)
				{ v0=v1; v1=d0; }
			else if (num == 64)
				{ v0=d0; v1=d1; }
			else if (num > 32) /* && num != 64 */
				{
				v0=((v1>>(num-32))|(d0<<(64-num)))&0xffffffffL;
				v1=((d0>>(num-32))|(d1<<(64-num)))&0xffffffffL;
				}
			else /* num < 32 */
				{
				v0=((v0>>num)|(v1<<(32-num)))&0xffffffffL;
				v1=((v1>>num)|(d0<<(32-num)))&0xffffffffL;
				}
			}
		}
	else
		{
		while (l >= n)
			{
			l-=n;
			ti[0]=v0;
			ti[1]=v1;
			des_encrypt((unsigned long *)ti,schedule,DES_ENCRYPT);
			c2ln(in,d0,d1,n);
			in+=n;
			/* 30-08-94 - eay - changed because l>>32 and
			 * l<<32 are bad under gcc :-( */
			if (num == 32)
				{ v0=v1; v1=d0; }
			else if (num == 64)
				{ v0=d0; v1=d1; }
			else if (num > 32) /* && num != 64 */
				{
				v0=((v1>>(num-32))|(d0<<(64-num)))&0xffffffffL;
				v1=((d0>>(num-32))|(d1<<(64-num)))&0xffffffffL;
				}
			else /* num < 32 */
				{
				v0=((v0>>num)|(v1<<(32-num)))&0xffffffffL;
				v1=((v1>>num)|(d0<<(32-num)))&0xffffffffL;
				}
			d0=(d0^ti[0])&mask0;
			d1=(d1^ti[1])&mask1;
			l2cn(d0,d1,out,n);
			out+=n;
			}
		}
	iv=(unsigned char *)ivec;
	l2c(v0,iv);
	l2c(v1,iv);
	v0=v1=d0=d1=ti[0]=ti[1]=0;
	}


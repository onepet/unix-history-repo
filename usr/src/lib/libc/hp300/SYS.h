/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)SYS.h	8.1 (Berkeley) 6/4/93
 */

#include <sys/syscall.h>

#ifdef __STDC__
#ifdef PROF
#if __GNUC__ >= 2
#define	ENTRY(x)	.globl _ ## x; .even; _ ## x:; .data; PROF ## x:; \
			.long 0; .text; link a6,\#0; lea PROF ## x,a0; \
			jbsr mcount; unlk a6
#else
#define	ENTRY(x)	.globl _ ## x; .even; _ ## x:; .data; PROF ## x:; \
			.long 0; .text; link a6,#0; lea PROF ## x,a0; \
			jbsr mcount; unlk a6
#endif
#else
#define	ENTRY(x)	.globl _ ## x; .even; _ ## x:
#endif
#if __GNUC__ >= 2
#define SYSTRAP(x)	movl \#SYS_ ## x,d0; trap \#0
#else
#define SYSTRAP(x)	movl #SYS_ ## x,d0; trap #0
#endif
#else
#ifdef PROF
#define	ENTRY(x)	.globl _/**/x; .even; _/**/x:; .data; PROF/**/x:; \
			.long 0; .text; link a6,#0; lea PROF/**/x,a0; \
			jbsr mcount; unlk a6
#else
#define	ENTRY(x)	.globl _/**/x; .even; _/**/x:
#endif
#define SYSTRAP(x)	movl #SYS_/**/x,d0; trap #0
#endif

#define	SYSCALL(x)	.even; err: jmp cerror; ENTRY(x); SYSTRAP(x); jcs err
#define	RSYSCALL(x)	SYSCALL(x); rts
#define	PSEUDO(x,y)	ENTRY(x); SYSTRAP(y); rts

#define	ASMSTR		.asciz

	.globl	cerror

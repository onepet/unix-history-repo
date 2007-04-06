/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _OPENSOLARIS_SYS_CMN_ERR_H_
#define	_OPENSOLARIS_SYS_CMN_ERR_H_

#include <sys/systm.h>
#include <machine/stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif

/* Common error handling severity levels */

#define	CE_CONT		0	/* continuation		*/
#define	CE_NOTE		1	/* notice		*/
#define	CE_WARN		2	/* warning		*/
#define	CE_PANIC	3	/* panic		*/
#define	CE_IGNORE	4	/* print nothing	*/

static __inline void
vcmn_err(int ce, const char *fmt, va_list adx)
{
	char buf[256];

	switch (ce) {
	case CE_CONT:
		snprintf(buf, sizeof(buf), "ZFS(cont): %s\n", fmt);
		break;
	case CE_NOTE:
		snprintf(buf, sizeof(buf), "ZFS: NOTICE: %s\n", fmt);
		break;
	case CE_WARN:
		snprintf(buf, sizeof(buf), "ZFS: WARNING: %s\n", fmt);
		break;
	case CE_PANIC:
		snprintf(buf, sizeof(buf), "ZFS(panic): %s\n", fmt);
		break;
	case CE_IGNORE:
		break;
	default:
		panic("unknown severity level");
	}
	if (ce != CE_IGNORE)
		vprintf(buf, adx);
	if (ce == CE_PANIC)
		panic("ZFS");
}

static __inline void
cmn_err(int ce, const char *fmt, ...)
{
	va_list adx;

	va_start(adx, fmt);
	vcmn_err(ce, fmt, adx);
	va_end(adx);
}

#ifdef	__cplusplus
}
#endif

#endif	/* _OPENSOLARIS_SYS_CMN_ERR_H_ */

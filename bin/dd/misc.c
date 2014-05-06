/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego and Lance
 * Visser of Convex Computer Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#ifndef lint
#if 0
static char sccsid[] = "@(#)misc.c	8.3 (Berkeley) 4/2/94";
#endif
#endif /* not lint */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#include "dd.h"
#include "extern.h"

void
summary(void)
{
	struct timespec tv, tv_res;
	double secs, res;

	if (ddflags & C_NOINFO)
		return;

	if (clock_gettime(CLOCK_MONOTONIC_PRECISE, &tv))
		err(EX_OSERR, "clock_gettime");
	if (clock_getres(CLOCK_MONOTONIC_PRECISE, &tv_res))
		err(EX_OSERR, "clock_getres");
	secs = tv.tv_sec + tv.tv_nsec * 1.0e-9 - st.start;
	res = tv_res.tv_sec + tv_res.tv_nsec * 1.0e-9;
	if (secs < res)
		secs = res;
	(void)fprintf(stderr,
	    "%ju+%ju records in\n%ju+%ju records out\n",
	    st.in_full, st.in_part, st.out_full, st.out_part);
	if (st.swab)
		(void)fprintf(stderr, "%ju odd length swab %s\n",
		     st.swab, (st.swab == 1) ? "block" : "blocks");
	if (st.trunc)
		(void)fprintf(stderr, "%ju truncated %s\n",
		     st.trunc, (st.trunc == 1) ? "block" : "blocks");
	if (!(ddflags & C_NOXFER)) {
		(void)fprintf(stderr,
		    "%ju bytes transferred in %.9f secs (%.0f bytes/sec)\n",
		    st.bytes, secs, st.bytes / secs);
	}
	need_summary = 0;
}

/* ARGSUSED */
void
siginfo_handler(int signo __unused)
{

	need_summary = 1;
}

/* ARGSUSED */
void
terminate(int sig)
{

	summary();
	_exit(sig == 0 ? 0 : 1);
}

/*-
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#ifndef lint
static char sccsid[] = "@(#)putsi.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

#include <stdio.h>
putsi(a){
	putc((char)a,stdout);
	putc((char)(a>>8),stdout);
}

/*-
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This module is believed to contain source code proprietary to AT&T.
 * Use and redistribution is subject to the Berkeley Software License
 * Agreement and your Software Agreement with AT&T (Western Electric).
 */

#ifndef lint
static char sccsid[] = "@(#)space.c	8.1 (Berkeley) 6/4/93";
#endif /* not lint */

#include "dumb.h"

space(x0, y0, x1, y1)
	int x0, y0, x1, y1;
{
	minX = x0;
	rangeX = x1 -x0;
	minY = y0;
	rangeY = y1 - y0;
}

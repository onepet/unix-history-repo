#ifndef lint
static char sccsid[] = "@(#)linmod.c	4.1 (Berkeley) %G%";
#endif

#include <stdio.h>
linemod(s)
char *s;
{
	int i;
	putc('f',stdout);
	for(i=0;s[i];)putc(s[i++],stdout);
	putc('\n',stdout);
}

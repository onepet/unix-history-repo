/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Cimarron D. Taylor of the University of California, Berkeley.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "@(#)misc.c	5.4 (Berkeley) %G%";
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include "find.h"
 
/*
 * find_subst --
 *	Replace occurrences of {} in s1 with s2 and return the result string.
 */
find_subst(orig, store, path, len)
	char *orig, **store, *path;
	int len;
{
	register int plen;
	register char ch, *p;
	char *realloc(), *strerror();

	plen = strlen(path);
	for (p = *store; ch = *orig; ++orig)
		if (ch == '{' && orig[1] == '}') {
			while ((p - *store) + plen > len)
				if (!(*store = realloc(*store, len *= 2))) {
					(void)fprintf(stderr,
					    "find: %s.\n", strerror(errno));
					exit(1);
				}
			bcopy(path, p, plen);
			p += plen;
			++orig;
		} else
			*p++ = ch;
	*p = '\0';
}

/*
 * find_queryuser --
 *	print a message to standard error and then read input from standard
 *	input. If the input is 'y' then 1 is returned.
 */
find_queryuser(argv)
	register char **argv;
{
	char buf[10];

	(void)fprintf(stderr, "\"%s", *argv);
	while (*++argv)
		(void)fprintf(stderr, " %s", *argv);
	(void)fprintf(stderr, "\"? ");
	(void)fflush(stderr);
	return(!fgets(buf, sizeof(buf), stdin) || buf[0] != 'y' ? 0 : 1);
}
 
/*
 * bad_arg --
 *	print out a bad argument message.
 */
void
bad_arg(option, error)
	char *option, *error;
{
	(void)fprintf(stderr, "find: %s: %s.\n", option, error);
	exit(1);
}
 
/*
 * emalloc --
 *	malloc with error checking.
 */
char *
emalloc(len)
	u_int len;
{
	char *p, *malloc(), *strerror();

	if (!(p = malloc(len))) {
		(void)fprintf(stderr, "find: %s.\n", strerror(errno));
		exit(1);
	}
	return(p);
}

usage()
{
	if (isdeprecated)
		(void)fprintf(stderr, "usage: find path-list expression\n");
	else
		(void)fprintf(stderr,
		    "usage: find [-drsx] -f path ... expression\n");
	exit(1);
}

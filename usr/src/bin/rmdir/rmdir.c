/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rmdir.c	5.3 (Berkeley) 5/31/90";
#endif /* not lint */

/*
 * Remove directory
 */
#include <stdio.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int errors;

	if (argc < 2) {
		fprintf(stderr, "usage: rmdir directory ...\n");
		exit(1);
	}
	for (errors = 0; *++argv;)
		if (rmdir(*argv) < 0) {
			fprintf(stderr, "rmdir: ");
			perror(*argv);
			errors = 1;
		}
	exit(errors);
}

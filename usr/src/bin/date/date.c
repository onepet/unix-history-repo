/*
 * Copyright (c) 1985, 1987, 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1985, 1987, 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)date.c	5.8 (Berkeley) %G%";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <syslog.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

time_t tval;
int retval, nflag;

main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	extern char *optarg;
	struct timezone tz;
	int ch, rflag;
	char *format, buf[1024];

	tz.tz_dsttime = tz.tz_minuteswest = 0;
	rflag = 0;
	while ((ch = getopt(argc, argv, "d:nr:ut:")) != EOF)
		switch((char)ch) {
		case 'd':		/* daylight savings time */
			tz.tz_dsttime = atoi(optarg) ? 1 : 0;
			break;
		case 'n':		/* don't set network */
			nflag = 1;
			break;
		case 'r':		/* user specified seconds */
			rflag = 1;
			tval = atol(optarg);
			break;
		case 'u':		/* do everything in GMT */
			(void)setenv("TZ", "GMT0", 1);
			break;
		case 't':		/* minutes west of GMT */
					/* error check; don't allow "PST" */
			if (isdigit(*optarg)) {
				tz.tz_minuteswest = atoi(optarg);
				break;
			}
			/* FALLTHROUGH */
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * If -d or -t, set the timezone or daylight savings time; this
	 * doesn't belong here, there kernel should not know about either.
	 */
	if ((tz.tz_minuteswest || tz.tz_dsttime) &&
	    settimeofday(NULL, &tz)) {
		perror("date: settimeofday");
		exit(1);
	}

	if (!rflag && time(&tval) == -1) {
		perror("date: time");
		exit(1);
	}

	format = "%a %b %e %H:%M:%S %Z %Y\n";

	/* allow the operands in any order */
	if (*argv && **argv == '+') {
		format = *argv + 1;
		++argv;
	}

	if (*argv) {
		setthetime(*argv);
		++argv;
	}

	if (*argv && **argv == '+')
		format = *argv + 1;

	(void)strftime(buf, sizeof(buf), format, localtime(&tval));
	(void)printf("%s", buf);
	exit(retval);
}

#define	ATOI2(ar)	((ar)[0] - '0') * 10 + ((ar)[1] - '0'); (ar) += 2;
setthetime(p)
	register char *p;
{
	register struct tm *lt;
	struct timeval tv;
	char *dot, *t;

	for (t = p, dot = NULL; *t; ++t) {
		if (isdigit(*t))
			continue;
		if (*t == '.' && dot == NULL) {
			dot = t;
			continue;
		}
		badformat();
	}

	lt = localtime(&tval);

	if (dot != NULL) {			/* .ss */
		*dot++ = '\0';
		if (strlen(dot) != 2)
			badformat();
		lt->tm_sec = ATOI2(dot);
		if (lt->tm_sec > 61)
			badformat();
	} else
		lt->tm_sec = 0;

	switch (strlen(p)) {
	case 10:				/* yy */
		lt->tm_year = ATOI2(p);
		if (lt->tm_year < 69)		/* hack for 2000 ;-} */
			lt->tm_year += 100;
		/* FALLTHROUGH */
	case 8:					/* mm */
		lt->tm_mon = ATOI2(p);
		if (lt->tm_mon > 12)
			badformat();
		--lt->tm_mon;			/* time struct is 0 - 11 */
		/* FALLTHROUGH */
	case 6:					/* dd */
		lt->tm_mday = ATOI2(p);
		if (lt->tm_mday > 31)
			badformat();
		/* FALLTHROUGH */
	case 4:					/* hh */
		lt->tm_hour = ATOI2(p);
		if (lt->tm_hour > 23)
			badformat();
		/* FALLTHROUGH */
	case 2:					/* mm */
		lt->tm_min = ATOI2(p);
		if (lt->tm_min > 59)
			badformat();
		break;
	default:
		badformat();
	}

	/* convert broken-down time to GMT clock time */
	if ((tval = mktime(lt)) == -1)
		badformat();

	/* set the time */
	if (nflag || netsettime(tval)) {
		logwtmp("|", "date", "");
		tv.tv_sec = tval;
		tv.tv_usec = 0;
		if (settimeofday(&tv, NULL)) {
			perror("date: settimeofday");
			exit(1);
		}
		logwtmp("{", "date", "");
	}

	if ((p = getlogin()) == NULL)
		p = "???";
	syslog(LOG_AUTH | LOG_NOTICE, "date set by %s", p);
}

badformat()
{
	(void)fprintf(stderr, "date: illegal time format.\n");
	usage();
}

usage()
{
	(void)fprintf(stderr,
	    "usage: date [-nu] [-d dst] [-r seconds] [-t west] [+format]\n");
	(void)fprintf(stderr, "            [yy[mm[dd[hh]]]]mm[.ss]]\n");
	exit(1);
}

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)rcmd.c	5.3 (Berkeley) %G%";
#endif not lint

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <netdb.h>
#include <errno.h>

extern	errno;
char	*index(), *sprintf();

rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	int rport;
	char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, timo = 1;
	struct sockaddr_in sin, sin2, from;
	char c;
	int lport = IPPORT_RESERVED - 1;
	struct hostent *hp;

	hp = gethostbyname(*ahost);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", *ahost);
		return (-1);
	}
	*ahost = hp->h_name;
retry:
	s = rresvport(&lport);
	if (s < 0)
		return (-1);
	sin.sin_family = hp->h_addrtype;
	bcopy(hp->h_addr, (caddr_t)&sin.sin_addr, hp->h_length);
	sin.sin_port = rport;
	if (connect(s, (caddr_t)&sin, sizeof (sin), 0) < 0) {
		(void) close(s);
		if (errno == EADDRINUSE) {
			lport--;
			goto retry;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			goto retry;
		}
		perror(hp->h_name);
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;
		int len = sizeof (from);

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			perror("write: setting up stderr");
			(void) close(s2);
			goto bad;
		}
		s3 = accept(s2, &from, &len, 0);
		(void) close(s2);
		if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		from.sin_port = ntohs((u_short)from.sin_port);
		if (from.sin_family != AF_INET ||
		    from.sin_port >= IPPORT_RESERVED) {
			fprintf(stderr,
			    "socket: protocol failure in circuit setup.\n");
			goto bad2;
		}
	}
	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	if (read(s, &c, 1) != 1) {
		perror(*ahost);
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	(void) close(s);
	return (-1);
}

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)*alport);
		if (bind(s, (caddr_t)&sin, sizeof (sin), 0) >= 0)
			return (s);
		if (errno == EADDRNOTAVAIL)
			return (-1);
		if (errno != EADDRINUSE) {
			perror("socket");
			(void) close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			fprintf(stderr, "socket: All ports in use\n");
			(void) close(s);
			return (-1);
		}
	}
}

ruserok(rhost, superuser, ruser, luser)
	char *rhost;
	int superuser;
	char *ruser, *luser;
{
	FILE *hostf;
	char fhost[MAXHOSTNAMELEN];
	char ahost[MAXHOSTNAMELEN];
	int first = 1;
	register char *sp, *p;
	int baselen = -1;

	sp = rhost;
	p = fhost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - rhost;
			*p++ = *sp++;
		} else {
			*p++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*p = '\0';
	hostf = superuser ? (FILE *)0 : fopen("/etc/hosts.equiv", "r");
again:
	if (hostf) {
		while (fgets(ahost, sizeof (ahost), hostf)) {
			char *user;

			p = ahost;
			while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
				*p++ = isupper(*p) ? tolower(*p) : *p;
			if (*p == ' ' || *p == '\t') {
				*p++ = '\0';
				while (*p == ' ' || *p == '\t')
					p++;
				user = p;
				while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0')
					p++;
			} else
				user = p;
			*p = '\0';
			if (_checkhost(fhost, ahost, baselen) &&
			    !strcmp(ruser, *user ? user : luser)) {
				(void) fclose(hostf);
				return (0);
			}
		}
		(void) fclose(hostf);
	}
	if (first == 1) {
		first = 0;
		hostf = fopen(".rhosts", "r");
		goto again;
	}
	return (-1);
}

_checkhost(rhost, lhost, len)
char *rhost, *lhost;
int len;
{
	static char ldomain[MAXHOSTNAMELEN];
	static char *domainp = NULL;
	register char *cp;

	if (len == -1)
		return(!strcmp(rhost, lhost));
	if (strncmp(rhost, lhost, len))
		return(0);
	if (!strcmp(rhost, lhost))
		return(1);
	if (*(lhost + len) != '\0')
		return(0);
	if (!domainp) {
		if (gethostname(ldomain, sizeof(ldomain)) == -1) {
			domainp = (char *)1;
			return(0);
		}
		ldomain[MAXHOSTNAMELEN] = NULL;
		domainp = index(ldomain, '.') + 1;
		cp = domainp;
		while (*cp)
			*cp++ = isupper(*cp) ? tolower(*cp) : *cp;
	}
	if (domainp == (char *)1)
		return(0);
	return(!strcmp(domainp, rhost + len +1));
}

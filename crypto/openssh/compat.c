/* $OpenBSD: compat.c,v 1.82 2013/12/30 23:52:27 djm Exp $ */
/* $FreeBSD$ */
/*
 * Copyright (c) 1999, 2000, 2001, 2002 Markus Friedl.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "buffer.h"
#include "packet.h"
#include "compat.h"
#include "log.h"
#include "match.h"

int compat13 = 0;
int compat20 = 0;
int datafellows = 0;

void
enable_compat20(void)
{
	if (compat20)
		return;
	debug("Enabling compatibility mode for protocol 2.0");
	compat20 = 1;
}
void
enable_compat13(void)
{
	debug("Enabling compatibility mode for protocol 1.3");
	compat13 = 1;
}
/* datafellows bug compatibility */
void
compat_datafellows(const char *version)
{
	int i;
	static struct {
		char	*pat;
		int	bugs;
	} check[] = {
		{ "OpenSSH-2.0*,"
		  "OpenSSH-2.1*,"
		  "OpenSSH_2.1*,"
		  "OpenSSH_2.2*",	SSH_OLD_SESSIONID|SSH_BUG_BANNER|
					SSH_OLD_DHGEX|SSH_BUG_NOREKEY|
					SSH_BUG_EXTEOF|SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_2.3.0*",	SSH_BUG_BANNER|SSH_BUG_BIGENDIANAES|
					SSH_OLD_DHGEX|SSH_BUG_NOREKEY|
					SSH_BUG_EXTEOF|SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_2.3.*",	SSH_BUG_BIGENDIANAES|SSH_OLD_DHGEX|
					SSH_BUG_NOREKEY|SSH_BUG_EXTEOF|
					SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_2.5.0p1*,"
		  "OpenSSH_2.5.1p1*",
					SSH_BUG_BIGENDIANAES|SSH_OLD_DHGEX|
					SSH_BUG_NOREKEY|SSH_BUG_EXTEOF|
					SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_2.5.0*,"
		  "OpenSSH_2.5.1*,"
		  "OpenSSH_2.5.2*",	SSH_OLD_DHGEX|SSH_BUG_NOREKEY|
					SSH_BUG_EXTEOF|SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_2.5.3*",	SSH_BUG_NOREKEY|SSH_BUG_EXTEOF|
					SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_2.*,"
		  "OpenSSH_3.0*,"
		  "OpenSSH_3.1*",	SSH_BUG_EXTEOF|SSH_OLD_FORWARD_ADDR},
		{ "OpenSSH_3.*",	SSH_OLD_FORWARD_ADDR },
		{ "Sun_SSH_1.0*",	SSH_BUG_NOREKEY|SSH_BUG_EXTEOF},
		{ "OpenSSH_4*",		0 },
		{ "OpenSSH_5*",		SSH_NEW_OPENSSH|SSH_BUG_DYNAMIC_RPORT},
		{ "OpenSSH_6.6.1*",	SSH_NEW_OPENSSH},
		{ "OpenSSH_6.5*,"
		  "OpenSSH_6.6*",	SSH_NEW_OPENSSH|SSH_BUG_CURVE25519PAD},
		{ "OpenSSH*",		SSH_NEW_OPENSSH },
		{ "*MindTerm*",		0 },
		{ "2.1.0*",		SSH_BUG_SIGBLOB|SSH_BUG_HMAC|
					SSH_OLD_SESSIONID|SSH_BUG_DEBUG|
					SSH_BUG_RSASIGMD5|SSH_BUG_HBSERVICE|
					SSH_BUG_FIRSTKEX },
		{ "2.1 *",		SSH_BUG_SIGBLOB|SSH_BUG_HMAC|
					SSH_OLD_SESSIONID|SSH_BUG_DEBUG|
					SSH_BUG_RSASIGMD5|SSH_BUG_HBSERVICE|
					SSH_BUG_FIRSTKEX },
		{ "2.0.13*,"
		  "2.0.14*,"
		  "2.0.15*,"
		  "2.0.16*,"
		  "2.0.17*,"
		  "2.0.18*,"
		  "2.0.19*",		SSH_BUG_SIGBLOB|SSH_BUG_HMAC|
					SSH_OLD_SESSIONID|SSH_BUG_DEBUG|
					SSH_BUG_PKSERVICE|SSH_BUG_X11FWD|
					SSH_BUG_PKOK|SSH_BUG_RSASIGMD5|
					SSH_BUG_HBSERVICE|SSH_BUG_OPENFAILURE|
					SSH_BUG_DUMMYCHAN|SSH_BUG_FIRSTKEX },
		{ "2.0.11*,"
		  "2.0.12*",		SSH_BUG_SIGBLOB|SSH_BUG_HMAC|
					SSH_OLD_SESSIONID|SSH_BUG_DEBUG|
					SSH_BUG_PKSERVICE|SSH_BUG_X11FWD|
					SSH_BUG_PKAUTH|SSH_BUG_PKOK|
					SSH_BUG_RSASIGMD5|SSH_BUG_OPENFAILURE|
					SSH_BUG_DUMMYCHAN|SSH_BUG_FIRSTKEX },
		{ "2.0.*",		SSH_BUG_SIGBLOB|SSH_BUG_HMAC|
					SSH_OLD_SESSIONID|SSH_BUG_DEBUG|
					SSH_BUG_PKSERVICE|SSH_BUG_X11FWD|
					SSH_BUG_PKAUTH|SSH_BUG_PKOK|
					SSH_BUG_RSASIGMD5|SSH_BUG_OPENFAILURE|
					SSH_BUG_DERIVEKEY|SSH_BUG_DUMMYCHAN|
					SSH_BUG_FIRSTKEX },
		{ "2.2.0*,"
		  "2.3.0*",		SSH_BUG_HMAC|SSH_BUG_DEBUG|
					SSH_BUG_RSASIGMD5|SSH_BUG_FIRSTKEX },
		{ "2.3.*",		SSH_BUG_DEBUG|SSH_BUG_RSASIGMD5|
					SSH_BUG_FIRSTKEX },
		{ "2.4",		SSH_OLD_SESSIONID },	/* Van Dyke */
		{ "2.*",		SSH_BUG_DEBUG|SSH_BUG_FIRSTKEX|
					SSH_BUG_RFWD_ADDR },
		{ "3.0.*",		SSH_BUG_DEBUG },
		{ "3.0 SecureCRT*",	SSH_OLD_SESSIONID },
		{ "1.7 SecureFX*",	SSH_OLD_SESSIONID },
		{ "1.2.18*,"
		  "1.2.19*,"
		  "1.2.20*,"
		  "1.2.21*,"
		  "1.2.22*",		SSH_BUG_IGNOREMSG },
		{ "1.3.2*",		/* F-Secure */
					SSH_BUG_IGNOREMSG },
		{ "*SSH Compatible Server*",			/* Netscreen */
					SSH_BUG_PASSWORDPAD },
		{ "*OSU_0*,"
		  "OSU_1.0*,"
		  "OSU_1.1*,"
		  "OSU_1.2*,"
		  "OSU_1.3*,"
		  "OSU_1.4*,"
		  "OSU_1.5alpha1*,"
		  "OSU_1.5alpha2*,"
		  "OSU_1.5alpha3*",	SSH_BUG_PASSWORDPAD },
		{ "*SSH_Version_Mapper*",
					SSH_BUG_SCANNER },
		{ "Probe-*",
					SSH_BUG_PROBE },
		{ NULL,			0 }
	};

	/* process table, return first match */
	for (i = 0; check[i].pat; i++) {
		if (match_pattern_list(version, check[i].pat,
		    strlen(check[i].pat), 0) == 1) {
			datafellows = check[i].bugs;
			debug("match: %s pat %s compat 0x%08x",
			    version, check[i].pat, datafellows);
			/*
			 * Check to see if the remote side is OpenSSH and not
			 * HPN.  It is utterly strange to check it from the
			 * version string and expose the option that way.
			 */
			if (strstr(version,"OpenSSH") != NULL &&
			    strstr(version,"hpn") == NULL) {
				datafellows |= SSH_BUG_LARGEWINDOW;
				debug("Remote is not HPN-aware");
			}
			return;
		}
	}
	debug("no match: %s", version);
}

#define	SEP	","
int
proto_spec(const char *spec)
{
	char *s, *p, *q;
	int ret = SSH_PROTO_UNKNOWN;

	if (spec == NULL)
		return ret;
	q = s = xstrdup(spec);
	for ((p = strsep(&q, SEP)); p && *p != '\0'; (p = strsep(&q, SEP))) {
		switch (atoi(p)) {
		case 1:
			if (ret == SSH_PROTO_UNKNOWN)
				ret |= SSH_PROTO_1_PREFERRED;
			ret |= SSH_PROTO_1;
			break;
		case 2:
			ret |= SSH_PROTO_2;
			break;
		default:
			logit("ignoring bad proto spec: '%s'.", p);
			break;
		}
	}
	free(s);
	return ret;
}

/*
 * Filters a proposal string, excluding any algorithm matching the 'filter'
 * pattern list.
 */
static char *
filter_proposal(char *proposal, const char *filter)
{
	Buffer b;
	char *orig_prop, *fix_prop;
	char *cp, *tmp;

	buffer_init(&b);
	tmp = orig_prop = xstrdup(proposal);
	while ((cp = strsep(&tmp, ",")) != NULL) {
		if (match_pattern_list(cp, filter, strlen(cp), 0) != 1) {
			if (buffer_len(&b) > 0)
				buffer_append(&b, ",", 1);
			buffer_append(&b, cp, strlen(cp));
		} else
			debug2("Compat: skipping algorithm \"%s\"", cp);
	}
	buffer_append(&b, "\0", 1);
	fix_prop = xstrdup(buffer_ptr(&b));
	buffer_free(&b);
	free(orig_prop);

	return fix_prop;
}

char *
compat_cipher_proposal(char *cipher_prop)
{
	if (!(datafellows & SSH_BUG_BIGENDIANAES))
		return cipher_prop;
	debug2("%s: original cipher proposal: %s", __func__, cipher_prop);
	cipher_prop = filter_proposal(cipher_prop, "aes*");
	debug2("%s: compat cipher proposal: %s", __func__, cipher_prop);
	if (*cipher_prop == '\0')
		fatal("No supported ciphers found");
	return cipher_prop;
}

char *
compat_pkalg_proposal(char *pkalg_prop)
{
	if (!(datafellows & SSH_BUG_RSASIGMD5))
		return pkalg_prop;
	debug2("%s: original public key proposal: %s", __func__, pkalg_prop);
	pkalg_prop = filter_proposal(pkalg_prop, "ssh-rsa");
	debug2("%s: compat public key proposal: %s", __func__, pkalg_prop);
	if (*pkalg_prop == '\0')
		fatal("No supported PK algorithms found");
	return pkalg_prop;
}

char *
compat_kex_proposal(char *kex_prop)
{
	if (!(datafellows & SSH_BUG_CURVE25519PAD))
		return kex_prop;
	debug2("%s: original KEX proposal: %s", __func__, kex_prop);
	kex_prop = filter_proposal(kex_prop, "curve25519-sha256@libssh.org");
	debug2("%s: compat KEX proposal: %s", __func__, kex_prop);
	if (*kex_prop == '\0')
		fatal("No supported key exchange algorithms found");
	return kex_prop;
}


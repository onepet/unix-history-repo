/*-
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by ThinkSec AS and
 * NAI Labs, the Security Research Division of Network Associates, Inc.
 * under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as part of the
 * DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $P4: //depot/projects/openpam/lib/pam_start.c#13 $
 */

#include <stdlib.h>

#include <security/pam_appl.h>

#include "openpam_impl.h"

/*
 * XSSO 4.2.1
 * XSSO 6 page 89
 *
 * Initiate a PAM transaction
 */

int
pam_start(const char *service,
	const char *user,
	const struct pam_conv *pam_conv,
	pam_handle_t **pamh)
{
	struct pam_handle *ph;
	int r;

	if ((ph = calloc(1, sizeof *ph)) == NULL)
		return (PAM_BUF_ERR);
	if ((r = pam_set_item(ph, PAM_SERVICE, service)) != PAM_SUCCESS)
		goto fail;
	if ((r = pam_set_item(ph, PAM_USER, user)) != PAM_SUCCESS)
		goto fail;
	if ((r = pam_set_item(ph, PAM_CONV, pam_conv)) != PAM_SUCCESS)
		goto fail;

	r = openpam_configure(ph, service);
	if (r != PAM_SUCCESS && r != PAM_BUF_ERR)
		r = openpam_configure(ph, PAM_OTHER);
	if (r != PAM_SUCCESS)
		goto fail;

	*pamh = ph;
	openpam_log(PAM_LOG_DEBUG, "pam_start(\"%s\") succeeded", service);
	return (PAM_SUCCESS);

 fail:
	pam_end(ph, r);
	return (r);
}

/*
 * Error codes:
 *
 *	=openpam_configure
 *	=pam_set_item
 *	!PAM_SYMBOL_ERR
 *	PAM_BUF_ERR
 */

/**
 * The =pam_start function creates and initializes a PAM context.
 *
 * The =service argument specifies the name of the policy to apply, and is
 * stored in the =PAM_SERVICE item in the created context.
 *
 * The =user argument specifies the name of the target user - the user the
 * created context will serve to authenticate.
 * It is stored in the =PAM_USER item in the created context.
 *
 * The =pam_conv argument points to a =struct pam_conv describing the
 * conversation function to use.
 * This structure is defined as follows:
 *
 *     struct pam_conv {
 *          int   (*conv)(int, const struct pam_message **,
 *              struct pam_response **, void *);
 *          void   *appdata_ptr;
 *     };
 *
 * >pam_get_item
 * >pam_set_item
 * >pam_end
 */

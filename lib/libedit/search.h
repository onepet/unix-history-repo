/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Christos Zoulas of Cornell University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *
 *	@(#)search.h	8.1 (Berkeley) 6/4/93
 */

/*
 * el.search.h: Line and history searching utilities
 */
#ifndef _h_el_search
#define _h_el_search

#include "histedit.h"

typedef struct el_search_t {
    char *patbuf;		/* The pattern buffer		*/
    int  patlen;		/* Length of the pattern buffer	*/
    int  patdir;		/* Direction of the last search	*/
    int  chadir;		/* Character search direction	*/
    char chacha;		/* Character we are looking for	*/
} el_search_t;


protected int 		el_match	__P((const char *, const char *));
protected int		search_init	__P((EditLine *));
protected void		search_end	__P((EditLine *));
protected int		c_hmatch	__P((EditLine *, const char *));
protected void		c_setpat	__P((EditLine *));
protected el_action_t	ce_inc_search	__P((EditLine *, int));
protected el_action_t	cv_search	__P((EditLine *, int));
protected el_action_t	ce_search_line	__P((EditLine *, char *, int));
protected el_action_t	cv_repeat_srch	__P((EditLine *, int));
protected el_action_t	cv_csearch_back	__P((EditLine *, int, int, int));
protected el_action_t	cv_csearch_fwd	__P((EditLine *, int, int, int));

#endif /* _h_el_search */

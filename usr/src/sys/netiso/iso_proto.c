/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)iso_proto.c	8.2 (Berkeley) %G%
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/* $Header: iso_proto.c,v 4.4 88/09/08 08:38:42 hagens Exp $ 
 * $Source: /usr/argo/sys/netiso/RCS/iso_proto.c,v $ 
 *
 * iso_proto.c : protocol switch tables in the ISO domain
 *
 * ISO protocol family includes TP, CLTP, CLNP, 8208
 * TP and CLNP are implemented here.
 */

#ifdef	ISO
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>

#include <net/radix.h>

#include <netiso/iso.h>

void	clnp_init(),clnp_slowtimo(),clnp_drain();
int	clnp_output();
void	rclnp_input();
int	rclnp_output(), rclnp_ctloutput(), raw_usrreq();
int	clnp_usrreq();

int	tp_ctloutput(), tp_usrreq();
void	tp_init(), tp_fasttimo(), tp_slowtimo(), tp_drain();
void	tpclnp_ctlinput(), tpclnp_input();
void	cons_init(), tpcons_input();

void	isis_input();
void	esis_init(), esis_input(), esis_ctlinput();
int	esis_usrreq();
void	idrp_init(), idrp_input();
int	idrp_usrreq();
void	cltp_init(), cltp_input(), cltp_ctlinput();
int	cltp_usrreq(), cltp_output();

#ifdef TUBA
int	tuba_usrreq(), tuba_ctloutput();
void	tuba_init(), tuba_slowtimo(), tuba_fasttimo(), tuba_tcpinput();
#endif

struct protosw isosw[] = {
/*
 *  We need a datagram entry through which net mgmt programs can get
 *	to the iso_control procedure (iso ioctls). Thus, a minimal
 *	SOCK_DGRAM interface is provided here.
 *  THIS ONE MUST BE FIRST: Kludge city : socket() says if(!proto) call
 *  pffindtype, which gets the first entry that matches the type.
 *  sigh.
 */
{ SOCK_DGRAM,	&isodomain,		ISOPROTO_CLTP,		PR_ATOMIC|PR_ADDR,
	0,			cltp_output,	0,					0,
	cltp_usrreq,
	cltp_init,	0, 				0,					0
},

/*
 *	A datagram interface for clnp cannot co-exist with TP/CLNP
 *  because CLNP has no way to discriminate incoming TP packets from
 *  packets coming in for any other higher layer protocol.
 *  Old way: set it up so that pffindproto(... dgm, clnp) fails.
 *  New way: let pffindproto work (for x.25, thank you) but create
 *  	a clnp_usrreq() that returns error on PRU_ATTACH.
 */
{SOCK_DGRAM,	&isodomain,		ISOPROTO_CLNP,		0,
 0,				clnp_output,	0,					0,
 clnp_usrreq,
 clnp_init,		0,				clnp_slowtimo, 		clnp_drain,
},

/* raw clnp */
{ SOCK_RAW,		&isodomain,		ISOPROTO_RAW,		PR_ATOMIC|PR_ADDR,
  rclnp_input,	rclnp_output,	0,					rclnp_ctloutput,
  clnp_usrreq,
  0,			0,				0,					0
},

/* ES-IS protocol */
{ SOCK_DGRAM,	&isodomain,		ISOPROTO_ESIS,		PR_ATOMIC|PR_ADDR,
  esis_input,	0,				esis_ctlinput,		0,
  esis_usrreq,
  esis_init,	0,				0,					0
},

/* ISOPROTO_INTRAISIS */
{ SOCK_DGRAM,	&isodomain,		ISOPROTO_INTRAISIS,	PR_ATOMIC|PR_ADDR,
  isis_input,	0,				0,					0,
  esis_usrreq,
  0,			0,				0,					0
},

/* ISOPROTO_IDRP */
{ SOCK_DGRAM,	&isodomain,		ISOPROTO_IDRP,		PR_ATOMIC|PR_ADDR,
  idrp_input,	0,				0,					0,
  idrp_usrreq,
  idrp_init,	0,				0,					0
},

/* ISOPROTO_TP */
{ SOCK_SEQPACKET,	&isodomain,	ISOPROTO_TP,		PR_CONNREQUIRED|PR_WANTRCVD,
  tpclnp_input,	0,				tpclnp_ctlinput,	tp_ctloutput,
  tp_usrreq,
  tp_init,		tp_fasttimo,	tp_slowtimo,		tp_drain,
},

#ifdef TUBA
{ SOCK_STREAM,	&isodomain,		ISOPROTO_TCP,		PR_CONNREQUIRED|PR_WANTRCVD,
  tuba_tcpinput,	0,			0,					tuba_ctloutput,
  tuba_usrreq,
  tuba_init,	tuba_fasttimo,	tuba_fasttimo,		0
},
#endif

#ifdef TPCONS
/* ISOPROTO_TP */
{ SOCK_SEQPACKET,	&isodomain,	ISOPROTO_TP0,		PR_CONNREQUIRED|PR_WANTRCVD,
  tpcons_input,		0,			0,					tp_ctloutput,
  tp_usrreq,
  cons_init,		0,			0,					0,
},
#endif

};


struct domain isodomain = {
    AF_ISO, 			/* family */
	"iso-domain", 		/* name */
	0,					/* initialize routine */
	0,					/* externalize access rights */
	0,					/* dispose of internalized rights */
	isosw,				/* protosw */
	&isosw[sizeof(isosw)/sizeof(isosw[0])], /* NPROTOSW */
	0,					/* next */
	rn_inithead,		/* rtattach */
	48,					/* rtoffset */
	sizeof(struct sockaddr_iso) /* maxkeylen */
};
#endif	/* ISO */

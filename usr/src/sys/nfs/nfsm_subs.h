/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)nfsm_subs.h	7.3 (Berkeley) %G%
 */

/*
 * These macros do strange and peculiar things to mbuf chains for
 * the assistance of the nfs code. To attempt to use them for any
 * other purpose will be dangerous. (they make weird assumptions)
 */

/*
 * First define what the actual subs. return
 */
struct mbuf *nfsm_reqh();
struct vnode *nfs_fhtovp();

/*
 * To try and deal with different variants of mbuf.h, I have used the
 * following defs. If M_HASCL is not defined in an older the 4.4bsd mbuf.h,
 * you will have to use a different ifdef
 */
#ifdef M_HASCL
#define	NFSMCLGET(m, w)	MCLGET(m)
#define	NFSMGETHDR(m)	MGET(m, M_WAIT, MT_DATA)
#define	MHLEN		MLEN
#define	NFSMINOFF(m) \
		if (M_HASCL(m)) \
			(m)->m_off = ((int)MTOCL(m))-(int)(m); \
		else \
			(m)->m_off = MMINOFF
#define	NFSMADV(m, s)	(m)->m_off += (s)
#define	NFSMSIZ(m)	((M_HASCL(m))?MCLBYTES:MLEN)
#define	m_nextpkt	m_act
#define	NFSMCOPY(m, o, l, w)	m_copy((m), (o), (l))
#else
#define	M_HASCL(m)	((m)->m_flags & M_EXT)
#define	NFSMCLGET	MCLGET
#define	NFSMGETHDR(m) \
		MGETHDR(m, M_WAIT, MT_DATA); \
		(m)->m_pkthdr.len = 0; \
		(m)->m_pkthdr.rcvif = (struct ifnet *)0
#define	NFSMINOFF(m) \
		if (M_HASCL(m)) \
			(m)->m_data = (m)->m_ext.ext_buf; \
		else \
			(m)->m_data = (m)->m_dat
#define	NFSMADV(m, s)	(m)->m_data += (s)
#define	NFSMSIZ(m)	((M_HASCL(m))?MCLBYTES: \
				(((m)->m_flags & M_PKTHDR)?MHLEN:MLEN))
#define	NFSMCOPY	m_copym
#endif

#ifndef MCLBYTES
#define	MCLBYTES	CLBYTES
#endif

#ifndef MT_CONTROL
#define	MT_CONTROL	MT_RIGHTS
#endif

/*
 * Now for the macros that do the simple stuff and call the functions
 * for the hard stuff.
 * These macros use several vars. declared in nfsm_reqhead and these
 * vars. must not be used elsewhere unless you are careful not to corrupt
 * them. The vars. starting with pN and tN (N=1,2,3,..) are temporaries
 * that may be used so long as the value is not expected to retained
 * after a macro.
 * I know, this is kind of dorkey, but it makes the actual op functions
 * fairly clean and deals with the mess caused by the xdr discriminating
 * unions.
 */

#define	nfsm_build(a,c,s) \
		t1 = NFSMSIZ(mb); \
		if ((s) > (t1-mb->m_len)) { \
			MGET(mb2, M_WAIT, MT_DATA); \
			if ((s) > MLEN) \
				panic("build > MLEN"); \
			mb->m_next = mb2; \
			mb = mb2; \
			mb->m_len = 0; \
			bpos = mtod(mb, caddr_t); \
		} \
		(a) = (c)(bpos); \
		mb->m_len += (s); \
		bpos += (s)

#define	nfsm_disect(a,c,s) \
		t1 = mtod(md, caddr_t)+md->m_len-dpos; \
		if (t1 >= (s)) { \
			(a) = (c)(dpos); \
			dpos += (s); \
		} else if (error = nfsm_disct(&md, &dpos, (s), t1, TRUE, &cp2)) { \
			m_freem(mrep); \
			goto nfsmout; \
		} else { \
			(a) = (c)cp2; \
		}

#define	nfsm_disecton(a,c,s) \
		t1 = mtod(md, caddr_t)+md->m_len-dpos; \
		if (t1 >= (s)) { \
			(a) = (c)(dpos); \
			dpos += (s); \
		} else if (error = nfsm_disct(&md, &dpos, (s), t1, FALSE, &cp2)) { \
			m_freem(mrep); \
			goto nfsmout; \
		} else { \
			(a) = (c)cp2; \
		}

#define nfsm_fhtom(v) \
		nfsm_build(cp,caddr_t,NFSX_FH); \
		bcopy((caddr_t)&(VTONFS(v)->n_fh), cp, NFSX_FH)

#define nfsm_srvfhtom(f) \
		nfsm_build(cp,caddr_t,NFSX_FH); \
		bcopy((caddr_t)(f), cp, NFSX_FH)

#define nfsm_mtofh(d,v) \
		{ struct nfsnode *np; nfsv2fh_t *fhp; \
		nfsm_disect(fhp,nfsv2fh_t *,NFSX_FH); \
		if (error = nfs_nget((d)->v_mount, fhp, &np)) { \
			m_freem(mrep); \
			goto nfsmout; \
		} \
		(v) = NFSTOV(np); \
		nfsm_loadattr(v, (struct vattr *)0); \
		(v)->v_type = np->n_vattr.va_type; \
		}

#define	nfsm_loadattr(v,a) \
		if (error = nfs_loadattrcache(&(v), &md, &dpos, (a))) { \
			m_freem(mrep); \
			goto nfsmout; \
		}

#define	nfsm_strsiz(s,m) \
		nfsm_disect(p,u_long *,NFSX_UNSIGNED); \
		if (((s) = fxdr_unsigned(long,*p)) > (m)) { \
			m_freem(mrep); \
			error = EBADRPC; \
			goto nfsmout; \
		}

#define	nfsm_srvstrsiz(s,m) \
		nfsm_disect(p,u_long *,NFSX_UNSIGNED); \
		if (((s) = fxdr_unsigned(long,*p)) > (m) || (s) <= 0) { \
			error = EBADRPC; \
			nfsm_reply(0); \
		}

#define	nfsm_srvstrsizon(s,m) \
		nfsm_disecton(p,u_long *,NFSX_UNSIGNED); \
		if (((s) = fxdr_unsigned(long,*p)) > (m)) { \
			error = EBADRPC; \
			nfsm_reply(0); \
		}

#define nfsm_mtouio(p,s) \
		if ((s) > 0 && \
		   (error = nfsm_mbuftouio(&md,(p),(s),&dpos))) { \
			m_freem(mrep); \
			goto nfsmout; \
		}

#define nfsm_uiotom(p,s) \
		if (error = nfsm_uiotombuf((p),&mb,(s),&bpos)) { \
			m_freem(mreq); \
			goto nfsmout; \
		}

#define	nfsm_reqhead(a,c,s) \
		if ((mreq = nfsm_reqh(nfs_prog,nfs_vers,(a),(c),(s),&bpos,&mb,&xid)) == NULL) { \
			error = ENOBUFS; \
			goto nfsmout; \
		}

#define	nfsm_vars \
		register u_long *p; \
		register caddr_t cp; \
		register long t1, t2; \
		caddr_t bpos, dpos, cp2; \
		u_long xid; \
		int error = 0; \
		long offs = 0; \
		struct mbuf *mreq, *mrep, *md, *mb, *mb2

#define nfsm_reqdone	m_freem(mrep); \
		nfsmout: 

#define nfsm_rndup(a)	(((a)+3)&(~0x3))

#define	nfsm_request(v)	\
		if (error = nfs_request((v), mreq, xid, \
		   (v)->v_mount, &mrep, &md, &dpos)) \
			goto nfsmout

#define	nfsm_strtom(a,s,m) \
		if ((s) > (m)) { \
			m_freem(mreq); \
			error = ENAMETOOLONG; \
			goto nfsmout; \
		} \
		t2 = nfsm_rndup(s)+NFSX_UNSIGNED; \
		if(t2<=(NFSMSIZ(mb)-mb->m_len)){ \
			nfsm_build(p,u_long *,t2); \
			*p++ = txdr_unsigned(s); \
			*(p+((t2>>2)-2)) = 0; \
			bcopy((caddr_t)(a), (caddr_t)p, (s)); \
		} else if (error = nfsm_strtmbuf(&mb, &bpos, (a), (s))) { \
			m_freem(mreq); \
			goto nfsmout; \
		}

#define	nfsm_srverr \
		{ \
			m_freem(mrep); \
			return(ENOBUFS); \
		}

#define	nfsm_srvars \
		register caddr_t cp; \
		register u_long *p; \
		register long t1, t2; \
		caddr_t bpos; \
		long offs = 0; \
		int error = 0; \
		char *cp2; \
		struct mbuf *mb, *mb2, *mreq

#define	nfsm_srvdone \
		nfsmout: \
		return(error)

#define	nfsm_reply(s) \
		{ \
		if (error) \
			nfs_rephead(0, xid, error, mrq, &mb, &bpos); \
		else \
			nfs_rephead((s), xid, error, mrq, &mb, &bpos); \
		m_freem(mrep); \
		mreq = *mrq; \
		if (error) \
			return(0); \
		}

#define	nfsm_adv(s) \
		t1 = mtod(md, caddr_t)+md->m_len-dpos; \
		if (t1 >= (s)) { \
			dpos += (s); \
		} else if (error = nfs_adv(&md, &dpos, (s), t1)) { \
			m_freem(mrep); \
			goto nfsmout; \
		}

#define nfsm_srvmtofh(f) \
		nfsm_disecton(p, u_long *, NFSX_FH); \
		bcopy((caddr_t)p, (caddr_t)f, NFSX_FH)

#define	nfsm_clget \
		if (bp >= be) { \
			MGET(mp, M_WAIT, MT_DATA); \
			NFSMCLGET(mp, M_WAIT); \
			mp->m_len = NFSMSIZ(mp); \
			if (mp3 == NULL) \
				mp3 = mp2 = mp; \
			else { \
				mp2->m_next = mp; \
				mp2 = mp; \
			} \
			bp = mtod(mp, caddr_t); \
			be = bp+mp->m_len; \
		} \
		p = (u_long *)bp


/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 *	@(#)nfs_subs.c  8.8 (Berkeley) 5/22/95
 * $FreeBSD$
 */

/*
 * These functions support the macros and help fiddle mbuf chains for
 * the nfs op functions. They do things like create the rpc header and
 * copy data between mbuf chains and uio lists.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/bio.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/sysent.h>
#include <sys/syscall.h>

#include <vm/vm.h>
#include <vm/vm_object.h>
#include <vm/vm_extern.h>
#include <vm/vm_zone.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <nfs/xdr_subs.h>
#include <nfs/nfsm_subs.h>
#include <nfs/nfsmount.h>
#include <nfs/nqnfs.h>
#include <nfs/nfsrtt.h>

#include <netinet/in.h>

/*
 * Data items converted to xdr at startup, since they are constant
 * This is kinda hokey, but may save a little time doing byte swaps
 */
u_int32_t nfs_xdrneg1;
u_int32_t rpc_call, rpc_vers, rpc_reply, rpc_msgdenied, rpc_autherr,
	rpc_mismatch, rpc_auth_unix, rpc_msgaccepted,
	rpc_auth_kerb;
u_int32_t nfs_prog, nqnfs_prog, nfs_true, nfs_false;

/* And other global data */
static u_int32_t nfs_xid = 0;
static enum vtype nv2tov_type[8]= {
	VNON, VREG, VDIR, VBLK, VCHR, VLNK, VNON,  VNON 
};
enum vtype nv3tov_type[8]= {
	VNON, VREG, VDIR, VBLK, VCHR, VLNK, VSOCK, VFIFO
};

int nfs_ticks;
int nfs_pbuf_freecnt = -1;	/* start out unlimited */

struct nfs_reqq nfs_reqq;
struct nfssvc_sockhead nfssvc_sockhead;
int nfssvc_sockhead_flag;
struct nfsd_head nfsd_head;
int nfsd_head_flag;
struct nfs_bufq nfs_bufq;
struct nqtimerhead nqtimerhead;
struct nqfhhashhead *nqfhhashtbl;
u_long nqfhhash;

static void (*nfs_prev_lease_updatetime) __P((int));
static int nfs_prev_nfssvc_sy_narg;
static sy_call_t *nfs_prev_nfssvc_sy_call;

#ifndef NFS_NOSERVER

static vop_t *nfs_prev_vop_lease_check;

/*
 * Mapping of old NFS Version 2 RPC numbers to generic numbers.
 */
int nfsv3_procid[NFS_NPROCS] = {
	NFSPROC_NULL,
	NFSPROC_GETATTR,
	NFSPROC_SETATTR,
	NFSPROC_NOOP,
	NFSPROC_LOOKUP,
	NFSPROC_READLINK,
	NFSPROC_READ,
	NFSPROC_NOOP,
	NFSPROC_WRITE,
	NFSPROC_CREATE,
	NFSPROC_REMOVE,
	NFSPROC_RENAME,
	NFSPROC_LINK,
	NFSPROC_SYMLINK,
	NFSPROC_MKDIR,
	NFSPROC_RMDIR,
	NFSPROC_READDIR,
	NFSPROC_FSSTAT,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP,
	NFSPROC_NOOP
};

#endif /* NFS_NOSERVER */
/*
 * and the reverse mapping from generic to Version 2 procedure numbers
 */
int nfsv2_procid[NFS_NPROCS] = {
	NFSV2PROC_NULL,
	NFSV2PROC_GETATTR,
	NFSV2PROC_SETATTR,
	NFSV2PROC_LOOKUP,
	NFSV2PROC_NOOP,
	NFSV2PROC_READLINK,
	NFSV2PROC_READ,
	NFSV2PROC_WRITE,
	NFSV2PROC_CREATE,
	NFSV2PROC_MKDIR,
	NFSV2PROC_SYMLINK,
	NFSV2PROC_CREATE,
	NFSV2PROC_REMOVE,
	NFSV2PROC_RMDIR,
	NFSV2PROC_RENAME,
	NFSV2PROC_LINK,
	NFSV2PROC_READDIR,
	NFSV2PROC_NOOP,
	NFSV2PROC_STATFS,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
	NFSV2PROC_NOOP,
};

#ifndef NFS_NOSERVER
/*
 * Maps errno values to nfs error numbers.
 * Use NFSERR_IO as the catch all for ones not specifically defined in
 * RFC 1094.
 */
static u_char nfsrv_v2errmap[ELAST] = {
  NFSERR_PERM,	NFSERR_NOENT,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_NXIO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_ACCES,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_EXIST,	NFSERR_IO,	NFSERR_NODEV,	NFSERR_NOTDIR,
  NFSERR_ISDIR,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_FBIG,	NFSERR_NOSPC,	NFSERR_IO,	NFSERR_ROFS,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_NAMETOL,	NFSERR_IO,	NFSERR_IO,
  NFSERR_NOTEMPTY, NFSERR_IO,	NFSERR_IO,	NFSERR_DQUOT,	NFSERR_STALE,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,	NFSERR_IO,
  NFSERR_IO /* << Last is 86 */
};

/*
 * Maps errno values to nfs error numbers.
 * Although it is not obvious whether or not NFS clients really care if
 * a returned error value is in the specified list for the procedure, the
 * safest thing to do is filter them appropriately. For Version 2, the
 * X/Open XNFS document is the only specification that defines error values
 * for each RPC (The RFC simply lists all possible error values for all RPCs),
 * so I have decided to not do this for Version 2.
 * The first entry is the default error return and the rest are the valid
 * errors for that RPC in increasing numeric order.
 */
static short nfsv3err_null[] = {
	0,
	0,
};

static short nfsv3err_getattr[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_setattr[] = {
	NFSERR_IO,
	NFSERR_PERM,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOT_SYNC,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_lookup[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_NAMETOL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_access[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_readlink[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_read[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_NXIO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_write[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_INVAL,
	NFSERR_FBIG,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_create[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_mkdir[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_symlink[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_mknod[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	NFSERR_BADTYPE,
	0,
};

static short nfsv3err_remove[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_rmdir[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_NOTDIR,
	NFSERR_INVAL,
	NFSERR_ROFS,
	NFSERR_NAMETOL,
	NFSERR_NOTEMPTY,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_rename[] = {
	NFSERR_IO,
	NFSERR_NOENT,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_XDEV,
	NFSERR_NOTDIR,
	NFSERR_ISDIR,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_MLINK,
	NFSERR_NAMETOL,
	NFSERR_NOTEMPTY,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_link[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_EXIST,
	NFSERR_XDEV,
	NFSERR_NOTDIR,
	NFSERR_INVAL,
	NFSERR_NOSPC,
	NFSERR_ROFS,
	NFSERR_MLINK,
	NFSERR_NAMETOL,
	NFSERR_DQUOT,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_NOTSUPP,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_readdir[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_BAD_COOKIE,
	NFSERR_TOOSMALL,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_readdirplus[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_ACCES,
	NFSERR_NOTDIR,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_BAD_COOKIE,
	NFSERR_NOTSUPP,
	NFSERR_TOOSMALL,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_fsstat[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_fsinfo[] = {
	NFSERR_STALE,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_pathconf[] = {
	NFSERR_STALE,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short nfsv3err_commit[] = {
	NFSERR_IO,
	NFSERR_IO,
	NFSERR_STALE,
	NFSERR_BADHANDLE,
	NFSERR_SERVERFAULT,
	0,
};

static short *nfsrv_v3errmap[] = {
	nfsv3err_null,
	nfsv3err_getattr,
	nfsv3err_setattr,
	nfsv3err_lookup,
	nfsv3err_access,
	nfsv3err_readlink,
	nfsv3err_read,
	nfsv3err_write,
	nfsv3err_create,
	nfsv3err_mkdir,
	nfsv3err_symlink,
	nfsv3err_mknod,
	nfsv3err_remove,
	nfsv3err_rmdir,
	nfsv3err_rename,
	nfsv3err_link,
	nfsv3err_readdir,
	nfsv3err_readdirplus,
	nfsv3err_fsstat,
	nfsv3err_fsinfo,
	nfsv3err_pathconf,
	nfsv3err_commit,
};

#endif /* NFS_NOSERVER */

extern struct nfsrtt nfsrtt;
extern time_t nqnfsstarttime;
extern int nqsrv_clockskew;
extern int nqsrv_writeslack;
extern int nqsrv_maxlease;
extern struct nfsstats nfsstats;
extern int nqnfs_piggy[NFS_NPROCS];
extern nfstype nfsv2_type[9];
extern nfstype nfsv3_type[9];
extern struct nfsnodehashhead *nfsnodehashtbl;
extern u_long nfsnodehash;

struct nfssvc_args;
extern int nfssvc(struct proc *, struct nfssvc_args *, int *);

LIST_HEAD(nfsnodehashhead, nfsnode);

int nfs_webnamei __P((struct nameidata *, struct vnode *, struct proc *));

u_quad_t
nfs_curusec() 
{
	struct timeval tv;
	
	getmicrotime(&tv);
	return ((u_quad_t)tv.tv_sec * 1000000 + (u_quad_t)tv.tv_usec);
}

/*
 * Create the header for an rpc request packet
 * The hsiz is the size of the rest of the nfs request header.
 * (just used to decide if a cluster is a good idea)
 */
struct mbuf *
nfsm_reqh(vp, procid, hsiz, bposp)
	struct vnode *vp;
	u_long procid;
	int hsiz;
	caddr_t *bposp;
{
	register struct mbuf *mb;
	register u_int32_t *tl;
	register caddr_t bpos;
	struct mbuf *mb2;
	struct nfsmount *nmp;
	int nqflag;

	MGET(mb, M_TRYWAIT, MT_DATA);
	if (hsiz >= MINCLSIZE)
		MCLGET(mb, M_TRYWAIT);
	mb->m_len = 0;
	bpos = mtod(mb, caddr_t);

	/*
	 * For NQNFS, add lease request.
	 */
	if (vp) {
		nmp = VFSTONFS(vp->v_mount);
		if (nmp->nm_flag & NFSMNT_NQNFS) {
			nqflag = NQNFS_NEEDLEASE(vp, procid);
			if (nqflag) {
				nfsm_build(tl, u_int32_t *, 2*NFSX_UNSIGNED);
				*tl++ = txdr_unsigned(nqflag);
				*tl = txdr_unsigned(nmp->nm_leaseterm);
			} else {
				nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED);
				*tl = 0;
			}
		}
	}
	/* Finally, return values */
	*bposp = bpos;
	return (mb);
}

/*
 * Build the RPC header and fill in the authorization info.
 * The authorization string argument is only used when the credentials
 * come from outside of the kernel.
 * Returns the head of the mbuf list.
 */
struct mbuf *
nfsm_rpchead(cr, nmflag, procid, auth_type, auth_len, auth_str, verf_len,
	verf_str, mrest, mrest_len, mbp, xidp)
	register struct ucred *cr;
	int nmflag;
	int procid;
	int auth_type;
	int auth_len;
	char *auth_str;
	int verf_len;
	char *verf_str;
	struct mbuf *mrest;
	int mrest_len;
	struct mbuf **mbp;
	u_int32_t *xidp;
{
	register struct mbuf *mb;
	register u_int32_t *tl;
	register caddr_t bpos;
	register int i;
	struct mbuf *mreq, *mb2;
	int siz, grpsiz, authsiz;

	authsiz = nfsm_rndup(auth_len);
	MGETHDR(mb, M_TRYWAIT, MT_DATA);
	if ((authsiz + 10 * NFSX_UNSIGNED) >= MINCLSIZE) {
		MCLGET(mb, M_TRYWAIT);
	} else if ((authsiz + 10 * NFSX_UNSIGNED) < MHLEN) {
		MH_ALIGN(mb, authsiz + 10 * NFSX_UNSIGNED);
	} else {
		MH_ALIGN(mb, 8 * NFSX_UNSIGNED);
	}
	mb->m_len = 0;
	mreq = mb;
	bpos = mtod(mb, caddr_t);

	/*
	 * First the RPC header.
	 */
	nfsm_build(tl, u_int32_t *, 8 * NFSX_UNSIGNED);

	/* Get a pretty random xid to start with */
	if (!nfs_xid) 
		nfs_xid = random();
	/*
	 * Skip zero xid if it should ever happen.
	 */
	if (++nfs_xid == 0)
		nfs_xid++;

	*tl++ = *xidp = txdr_unsigned(nfs_xid);
	*tl++ = rpc_call;
	*tl++ = rpc_vers;
	if (nmflag & NFSMNT_NQNFS) {
		*tl++ = txdr_unsigned(NQNFS_PROG);
		*tl++ = txdr_unsigned(NQNFS_VER3);
	} else {
		*tl++ = txdr_unsigned(NFS_PROG);
		if (nmflag & NFSMNT_NFSV3)
			*tl++ = txdr_unsigned(NFS_VER3);
		else
			*tl++ = txdr_unsigned(NFS_VER2);
	}
	if (nmflag & NFSMNT_NFSV3)
		*tl++ = txdr_unsigned(procid);
	else
		*tl++ = txdr_unsigned(nfsv2_procid[procid]);

	/*
	 * And then the authorization cred.
	 */
	*tl++ = txdr_unsigned(auth_type);
	*tl = txdr_unsigned(authsiz);
	switch (auth_type) {
	case RPCAUTH_UNIX:
		nfsm_build(tl, u_int32_t *, auth_len);
		*tl++ = 0;		/* stamp ?? */
		*tl++ = 0;		/* NULL hostname */
		*tl++ = txdr_unsigned(cr->cr_uid);
		*tl++ = txdr_unsigned(cr->cr_groups[0]);
		grpsiz = (auth_len >> 2) - 5;
		*tl++ = txdr_unsigned(grpsiz);
		for (i = 1; i <= grpsiz; i++)
			*tl++ = txdr_unsigned(cr->cr_groups[i]);
		break;
	case RPCAUTH_KERB4:
		siz = auth_len;
		while (siz > 0) {
			if (M_TRAILINGSPACE(mb) == 0) {
				MGET(mb2, M_TRYWAIT, MT_DATA);
				if (siz >= MINCLSIZE)
					MCLGET(mb2, M_TRYWAIT);
				mb->m_next = mb2;
				mb = mb2;
				mb->m_len = 0;
				bpos = mtod(mb, caddr_t);
			}
			i = min(siz, M_TRAILINGSPACE(mb));
			bcopy(auth_str, bpos, i);
			mb->m_len += i;
			auth_str += i;
			bpos += i;
			siz -= i;
		}
		if ((siz = (nfsm_rndup(auth_len) - auth_len)) > 0) {
			for (i = 0; i < siz; i++)
				*bpos++ = '\0';
			mb->m_len += siz;
		}
		break;
	};

	/*
	 * And the verifier...
	 */
	nfsm_build(tl, u_int32_t *, 2 * NFSX_UNSIGNED);
	if (verf_str) {
		*tl++ = txdr_unsigned(RPCAUTH_KERB4);
		*tl = txdr_unsigned(verf_len);
		siz = verf_len;
		while (siz > 0) {
			if (M_TRAILINGSPACE(mb) == 0) {
				MGET(mb2, M_TRYWAIT, MT_DATA);
				if (siz >= MINCLSIZE)
					MCLGET(mb2, M_TRYWAIT);
				mb->m_next = mb2;
				mb = mb2;
				mb->m_len = 0;
				bpos = mtod(mb, caddr_t);
			}
			i = min(siz, M_TRAILINGSPACE(mb));
			bcopy(verf_str, bpos, i);
			mb->m_len += i;
			verf_str += i;
			bpos += i;
			siz -= i;
		}
		if ((siz = (nfsm_rndup(verf_len) - verf_len)) > 0) {
			for (i = 0; i < siz; i++)
				*bpos++ = '\0';
			mb->m_len += siz;
		}
	} else {
		*tl++ = txdr_unsigned(RPCAUTH_NULL);
		*tl = 0;
	}
	mb->m_next = mrest;
	mreq->m_pkthdr.len = authsiz + 10 * NFSX_UNSIGNED + mrest_len;
	mreq->m_pkthdr.rcvif = (struct ifnet *)0;
	*mbp = mb;
	return (mreq);
}

/*
 * copies mbuf chain to the uio scatter/gather list
 */
int
nfsm_mbuftouio(mrep, uiop, siz, dpos)
	struct mbuf **mrep;
	register struct uio *uiop;
	int siz;
	caddr_t *dpos;
{
	register char *mbufcp, *uiocp;
	register int xfer, left, len;
	register struct mbuf *mp;
	long uiosiz, rem;
	int error = 0;

	mp = *mrep;
	mbufcp = *dpos;
	len = mtod(mp, caddr_t)+mp->m_len-mbufcp;
	rem = nfsm_rndup(siz)-siz;
	while (siz > 0) {
		if (uiop->uio_iovcnt <= 0 || uiop->uio_iov == NULL)
			return (EFBIG);
		left = uiop->uio_iov->iov_len;
		uiocp = uiop->uio_iov->iov_base;
		if (left > siz)
			left = siz;
		uiosiz = left;
		while (left > 0) {
			while (len == 0) {
				mp = mp->m_next;
				if (mp == NULL)
					return (EBADRPC);
				mbufcp = mtod(mp, caddr_t);
				len = mp->m_len;
			}
			xfer = (left > len) ? len : left;
#ifdef notdef
			/* Not Yet.. */
			if (uiop->uio_iov->iov_op != NULL)
				(*(uiop->uio_iov->iov_op))
				(mbufcp, uiocp, xfer);
			else
#endif
			if (uiop->uio_segflg == UIO_SYSSPACE)
				bcopy(mbufcp, uiocp, xfer);
			else
				copyout(mbufcp, uiocp, xfer);
			left -= xfer;
			len -= xfer;
			mbufcp += xfer;
			uiocp += xfer;
			uiop->uio_offset += xfer;
			uiop->uio_resid -= xfer;
		}
		if (uiop->uio_iov->iov_len <= siz) {
			uiop->uio_iovcnt--;
			uiop->uio_iov++;
		} else {
			uiop->uio_iov->iov_base += uiosiz;
			uiop->uio_iov->iov_len -= uiosiz;
		}
		siz -= uiosiz;
	}
	*dpos = mbufcp;
	*mrep = mp;
	if (rem > 0) {
		if (len < rem)
			error = nfs_adv(mrep, dpos, rem, len);
		else
			*dpos += rem;
	}
	return (error);
}

/*
 * copies a uio scatter/gather list to an mbuf chain.
 * NOTE: can ony handle iovcnt == 1
 */
int
nfsm_uiotombuf(uiop, mq, siz, bpos)
	register struct uio *uiop;
	struct mbuf **mq;
	int siz;
	caddr_t *bpos;
{
	register char *uiocp;
	register struct mbuf *mp, *mp2;
	register int xfer, left, mlen;
	int uiosiz, clflg, rem;
	char *cp;

#ifdef DIAGNOSTIC
	if (uiop->uio_iovcnt != 1)
		panic("nfsm_uiotombuf: iovcnt != 1");
#endif

	if (siz > MLEN)		/* or should it >= MCLBYTES ?? */
		clflg = 1;
	else
		clflg = 0;
	rem = nfsm_rndup(siz)-siz;
	mp = mp2 = *mq;
	while (siz > 0) {
		left = uiop->uio_iov->iov_len;
		uiocp = uiop->uio_iov->iov_base;
		if (left > siz)
			left = siz;
		uiosiz = left;
		while (left > 0) {
			mlen = M_TRAILINGSPACE(mp);
			if (mlen == 0) {
				MGET(mp, M_TRYWAIT, MT_DATA);
				if (clflg)
					MCLGET(mp, M_TRYWAIT);
				mp->m_len = 0;
				mp2->m_next = mp;
				mp2 = mp;
				mlen = M_TRAILINGSPACE(mp);
			}
			xfer = (left > mlen) ? mlen : left;
#ifdef notdef
			/* Not Yet.. */
			if (uiop->uio_iov->iov_op != NULL)
				(*(uiop->uio_iov->iov_op))
				(uiocp, mtod(mp, caddr_t)+mp->m_len, xfer);
			else
#endif
			if (uiop->uio_segflg == UIO_SYSSPACE)
				bcopy(uiocp, mtod(mp, caddr_t)+mp->m_len, xfer);
			else
				copyin(uiocp, mtod(mp, caddr_t)+mp->m_len, xfer);
			mp->m_len += xfer;
			left -= xfer;
			uiocp += xfer;
			uiop->uio_offset += xfer;
			uiop->uio_resid -= xfer;
		}
		uiop->uio_iov->iov_base += uiosiz;
		uiop->uio_iov->iov_len -= uiosiz;
		siz -= uiosiz;
	}
	if (rem > 0) {
		if (rem > M_TRAILINGSPACE(mp)) {
			MGET(mp, M_TRYWAIT, MT_DATA);
			mp->m_len = 0;
			mp2->m_next = mp;
		}
		cp = mtod(mp, caddr_t)+mp->m_len;
		for (left = 0; left < rem; left++)
			*cp++ = '\0';
		mp->m_len += rem;
		*bpos = cp;
	} else
		*bpos = mtod(mp, caddr_t)+mp->m_len;
	*mq = mp;
	return (0);
}

/*
 * Help break down an mbuf chain by setting the first siz bytes contiguous
 * pointed to by returned val.
 * This is used by the macros nfsm_dissect and nfsm_dissecton for tough
 * cases. (The macros use the vars. dpos and dpos2)
 */
int
nfsm_disct(mdp, dposp, siz, left, cp2)
	struct mbuf **mdp;
	caddr_t *dposp;
	int siz;
	int left;
	caddr_t *cp2;
{
	register struct mbuf *mp, *mp2;
	register int siz2, xfer;
	register caddr_t p;

	mp = *mdp;
	while (left == 0) {
		*mdp = mp = mp->m_next;
		if (mp == NULL)
			return (EBADRPC);
		left = mp->m_len;
		*dposp = mtod(mp, caddr_t);
	}
	if (left >= siz) {
		*cp2 = *dposp;
		*dposp += siz;
	} else if (mp->m_next == NULL) {
		return (EBADRPC);
	} else if (siz > MHLEN) {
		panic("nfs S too big");
	} else {
		MGET(mp2, M_TRYWAIT, MT_DATA);
		mp2->m_next = mp->m_next;
		mp->m_next = mp2;
		mp->m_len -= left;
		mp = mp2;
		*cp2 = p = mtod(mp, caddr_t);
		bcopy(*dposp, p, left);		/* Copy what was left */
		siz2 = siz-left;
		p += left;
		mp2 = mp->m_next;
		/* Loop around copying up the siz2 bytes */
		while (siz2 > 0) {
			if (mp2 == NULL)
				return (EBADRPC);
			xfer = (siz2 > mp2->m_len) ? mp2->m_len : siz2;
			if (xfer > 0) {
				bcopy(mtod(mp2, caddr_t), p, xfer);
				NFSMADV(mp2, xfer);
				mp2->m_len -= xfer;
				p += xfer;
				siz2 -= xfer;
			}
			if (siz2 > 0)
				mp2 = mp2->m_next;
		}
		mp->m_len = siz;
		*mdp = mp2;
		*dposp = mtod(mp2, caddr_t);
	}
	return (0);
}

/*
 * Advance the position in the mbuf chain.
 */
int
nfs_adv(mdp, dposp, offs, left)
	struct mbuf **mdp;
	caddr_t *dposp;
	int offs;
	int left;
{
	register struct mbuf *m;
	register int s;

	m = *mdp;
	s = left;
	while (s < offs) {
		offs -= s;
		m = m->m_next;
		if (m == NULL)
			return (EBADRPC);
		s = m->m_len;
	}
	*mdp = m;
	*dposp = mtod(m, caddr_t)+offs;
	return (0);
}

/*
 * Copy a string into mbufs for the hard cases...
 */
int
nfsm_strtmbuf(mb, bpos, cp, siz)
	struct mbuf **mb;
	char **bpos;
	const char *cp;
	long siz;
{
	register struct mbuf *m1 = NULL, *m2;
	long left, xfer, len, tlen;
	u_int32_t *tl;
	int putsize;

	putsize = 1;
	m2 = *mb;
	left = M_TRAILINGSPACE(m2);
	if (left > 0) {
		tl = ((u_int32_t *)(*bpos));
		*tl++ = txdr_unsigned(siz);
		putsize = 0;
		left -= NFSX_UNSIGNED;
		m2->m_len += NFSX_UNSIGNED;
		if (left > 0) {
			bcopy(cp, (caddr_t) tl, left);
			siz -= left;
			cp += left;
			m2->m_len += left;
			left = 0;
		}
	}
	/* Loop around adding mbufs */
	while (siz > 0) {
		MGET(m1, M_TRYWAIT, MT_DATA);
		if (siz > MLEN)
			MCLGET(m1, M_TRYWAIT);
		m1->m_len = NFSMSIZ(m1);
		m2->m_next = m1;
		m2 = m1;
		tl = mtod(m1, u_int32_t *);
		tlen = 0;
		if (putsize) {
			*tl++ = txdr_unsigned(siz);
			m1->m_len -= NFSX_UNSIGNED;
			tlen = NFSX_UNSIGNED;
			putsize = 0;
		}
		if (siz < m1->m_len) {
			len = nfsm_rndup(siz);
			xfer = siz;
			if (xfer < len)
				*(tl+(xfer>>2)) = 0;
		} else {
			xfer = len = m1->m_len;
		}
		bcopy(cp, (caddr_t) tl, xfer);
		m1->m_len = len+tlen;
		siz -= xfer;
		cp += xfer;
	}
	*mb = m1;
	*bpos = mtod(m1, caddr_t)+m1->m_len;
	return (0);
}

/*
 * Called once to initialize data structures...
 */
int
nfs_init(vfsp)
	struct vfsconf *vfsp;
{
	register int i;

	nfsmount_zone = zinit("NFSMOUNT", sizeof(struct nfsmount), 0, 0, 1);

	nfs_mount_type = vfsp->vfc_typenum;
	nfsrtt.pos = 0;
	rpc_vers = txdr_unsigned(RPC_VER2);
	rpc_call = txdr_unsigned(RPC_CALL);
	rpc_reply = txdr_unsigned(RPC_REPLY);
	rpc_msgdenied = txdr_unsigned(RPC_MSGDENIED);
	rpc_msgaccepted = txdr_unsigned(RPC_MSGACCEPTED);
	rpc_mismatch = txdr_unsigned(RPC_MISMATCH);
	rpc_autherr = txdr_unsigned(RPC_AUTHERR);
	rpc_auth_unix = txdr_unsigned(RPCAUTH_UNIX);
	rpc_auth_kerb = txdr_unsigned(RPCAUTH_KERB4);
	nfs_prog = txdr_unsigned(NFS_PROG);
	nqnfs_prog = txdr_unsigned(NQNFS_PROG);
	nfs_true = txdr_unsigned(TRUE);
	nfs_false = txdr_unsigned(FALSE);
	nfs_xdrneg1 = txdr_unsigned(-1);
	nfs_ticks = (hz * NFS_TICKINTVL + 500) / 1000;
	if (nfs_ticks < 1)
		nfs_ticks = 1;
	/* Ensure async daemons disabled */
	for (i = 0; i < NFS_MAXASYNCDAEMON; i++) {
		nfs_iodwant[i] = (struct proc *)0;
		nfs_iodmount[i] = (struct nfsmount *)0;
	}
	nfs_nhinit();			/* Init the nfsnode table */
#ifndef NFS_NOSERVER
	nfsrv_init(0);			/* Init server data structures */
	nfsrv_initcache();		/* Init the server request cache */
#endif

	/*
	 * Initialize the nqnfs server stuff.
	 */
	if (nqnfsstarttime == 0) {
		nqnfsstarttime = boottime.tv_sec + nqsrv_maxlease
			+ nqsrv_clockskew + nqsrv_writeslack;
		NQLOADNOVRAM(nqnfsstarttime);
		TAILQ_INIT(&nqtimerhead);
		nqfhhashtbl = hashinit(NQLCHSZ, M_NQLEASE, &nqfhhash);
	}

	/*
	 * Initialize reply list and start timer
	 */
	TAILQ_INIT(&nfs_reqq);

	nfs_timer(0);

	/*
	 * Set up lease_check and lease_updatetime so that other parts
	 * of the system can call us, if we are loadable.
	 */
#ifndef NFS_NOSERVER
	nfs_prev_vop_lease_check = default_vnodeop_p[VOFFSET(vop_lease)];
	default_vnodeop_p[VOFFSET(vop_lease)] = (vop_t *)nqnfs_vop_lease_check;
#endif
	nfs_prev_lease_updatetime = lease_updatetime;
	lease_updatetime = nfs_lease_updatetime;
	nfs_prev_nfssvc_sy_narg = sysent[SYS_nfssvc].sy_narg;
	sysent[SYS_nfssvc].sy_narg = 2;
	nfs_prev_nfssvc_sy_call = sysent[SYS_nfssvc].sy_call;
	sysent[SYS_nfssvc].sy_call = (sy_call_t *)nfssvc;

	nfs_pbuf_freecnt = nswbuf / 2 + 1;

	return (0);
}

int
nfs_uninit(vfsp)
	struct vfsconf *vfsp;
{

	untimeout(nfs_timer, (void *)NULL, nfs_timer_handle);
	nfs_mount_type = -1;
#ifndef NFS_NOSERVER
	default_vnodeop_p[VOFFSET(vop_lease)] = nfs_prev_vop_lease_check;
#endif
	lease_updatetime = nfs_prev_lease_updatetime;
	sysent[SYS_nfssvc].sy_narg = nfs_prev_nfssvc_sy_narg;
	sysent[SYS_nfssvc].sy_call = nfs_prev_nfssvc_sy_call;
	return (0);
}

/*
 * Attribute cache routines.
 * nfs_loadattrcache() - loads or updates the cache contents from attributes
 *	that are on the mbuf list
 * nfs_getattrcache() - returns valid attributes if found in cache, returns
 *	error otherwise
 */

/*
 * Load the attribute cache (that lives in the nfsnode entry) with
 * the values on the mbuf list and
 * Iff vap not NULL
 *    copy the attributes to *vaper
 */
int
nfs_loadattrcache(vpp, mdp, dposp, vaper, dontshrink)
	struct vnode **vpp;
	struct mbuf **mdp;
	caddr_t *dposp;
	struct vattr *vaper;
	int dontshrink;
{
	register struct vnode *vp = *vpp;
	register struct vattr *vap;
	register struct nfs_fattr *fp;
	register struct nfsnode *np;
	register int32_t t1;
	caddr_t cp2;
	int error = 0, rdev;
	struct mbuf *md;
	enum vtype vtyp;
	u_short vmode;
	struct timespec mtime;
	int v3 = NFS_ISV3(vp);

	md = *mdp;
	t1 = (mtod(md, caddr_t) + md->m_len) - *dposp;
	if ((error = nfsm_disct(mdp, dposp, NFSX_FATTR(v3), t1, &cp2)) != 0)
		return (error);
	fp = (struct nfs_fattr *)cp2;
	if (v3) {
		vtyp = nfsv3tov_type(fp->fa_type);
		vmode = fxdr_unsigned(u_short, fp->fa_mode);
		rdev = makeudev(fxdr_unsigned(int, fp->fa3_rdev.specdata1),
			fxdr_unsigned(int, fp->fa3_rdev.specdata2));
		fxdr_nfsv3time(&fp->fa3_mtime, &mtime);
	} else {
		vtyp = nfsv2tov_type(fp->fa_type);
		vmode = fxdr_unsigned(u_short, fp->fa_mode);
		/*
		 * XXX
		 *
		 * The duplicate information returned in fa_type and fa_mode
		 * is an ambiguity in the NFS version 2 protocol.
		 *
		 * VREG should be taken literally as a regular file.  If a
		 * server intents to return some type information differently
		 * in the upper bits of the mode field (e.g. for sockets, or
		 * FIFOs), NFSv2 mandates fa_type to be VNON.  Anyway, we
		 * leave the examination of the mode bits even in the VREG
		 * case to avoid breakage for bogus servers, but we make sure
		 * that there are actually type bits set in the upper part of
		 * fa_mode (and failing that, trust the va_type field).
		 *
		 * NFSv3 cleared the issue, and requires fa_mode to not
		 * contain any type information (while also introduing sockets
		 * and FIFOs for fa_type).
		 */
		if (vtyp == VNON || (vtyp == VREG && (vmode & S_IFMT) != 0))
			vtyp = IFTOVT(vmode);
		rdev = fxdr_unsigned(int32_t, fp->fa2_rdev);
		fxdr_nfsv2time(&fp->fa2_mtime, &mtime);

		/*
		 * Really ugly NFSv2 kludge.
		 */
		if (vtyp == VCHR && rdev == 0xffffffff)
			vtyp = VFIFO;
	}

	/*
	 * If v_type == VNON it is a new node, so fill in the v_type,
	 * n_mtime fields. Check to see if it represents a special
	 * device, and if so, check for a possible alias. Once the
	 * correct vnode has been obtained, fill in the rest of the
	 * information.
	 */
	np = VTONFS(vp);
	if (vp->v_type != vtyp) {
		vp->v_type = vtyp;
		if (vp->v_type == VFIFO) {
			vp->v_op = fifo_nfsv2nodeop_p;
		}
		if (vp->v_type == VCHR || vp->v_type == VBLK) {
			vp->v_op = spec_nfsv2nodeop_p;
			vp = addaliasu(vp, rdev);
			np->n_vnode = vp;
		}
		np->n_mtime = mtime.tv_sec;
	}
	vap = &np->n_vattr;
	vap->va_type = vtyp;
	vap->va_mode = (vmode & 07777);
	vap->va_rdev = rdev;
	vap->va_mtime = mtime;
	vap->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
	if (v3) {
		vap->va_nlink = fxdr_unsigned(u_short, fp->fa_nlink);
		vap->va_uid = fxdr_unsigned(uid_t, fp->fa_uid);
		vap->va_gid = fxdr_unsigned(gid_t, fp->fa_gid);
		vap->va_size = fxdr_hyper(&fp->fa3_size);
		vap->va_blocksize = NFS_FABLKSIZE;
		vap->va_bytes = fxdr_hyper(&fp->fa3_used);
		vap->va_fileid = fxdr_unsigned(int32_t,
		    fp->fa3_fileid.nfsuquad[1]);
		fxdr_nfsv3time(&fp->fa3_atime, &vap->va_atime);
		fxdr_nfsv3time(&fp->fa3_ctime, &vap->va_ctime);
		vap->va_flags = 0;
		vap->va_filerev = 0;
	} else {
		vap->va_nlink = fxdr_unsigned(u_short, fp->fa_nlink);
		vap->va_uid = fxdr_unsigned(uid_t, fp->fa_uid);
		vap->va_gid = fxdr_unsigned(gid_t, fp->fa_gid);
		vap->va_size = fxdr_unsigned(u_int32_t, fp->fa2_size);
		vap->va_blocksize = fxdr_unsigned(int32_t, fp->fa2_blocksize);
		vap->va_bytes = (u_quad_t)fxdr_unsigned(int32_t, fp->fa2_blocks)
		    * NFS_FABLKSIZE;
		vap->va_fileid = fxdr_unsigned(int32_t, fp->fa2_fileid);
		fxdr_nfsv2time(&fp->fa2_atime, &vap->va_atime);
		vap->va_flags = 0;
		vap->va_ctime.tv_sec = fxdr_unsigned(u_int32_t,
		    fp->fa2_ctime.nfsv2_sec);
		vap->va_ctime.tv_nsec = 0;
		vap->va_gen = fxdr_unsigned(u_int32_t,fp->fa2_ctime.nfsv2_usec);
		vap->va_filerev = 0;
	}
	np->n_attrstamp = time_second;
	if (vap->va_size != np->n_size) {
		if (vap->va_type == VREG) {
			if (dontshrink && vap->va_size < np->n_size) {
				/*
				 * We've been told not to shrink the file;
				 * zero np->n_attrstamp to indicate that
				 * the attributes are stale.
				 */
				vap->va_size = np->n_size;
				np->n_attrstamp = 0;
			} else if (np->n_flag & NMODIFIED) {
				if (vap->va_size < np->n_size)
					vap->va_size = np->n_size;
				else
					np->n_size = vap->va_size;
			} else {
				np->n_size = vap->va_size;
			}
			vnode_pager_setsize(vp, np->n_size);
		} else {
			np->n_size = vap->va_size;
		}
	}
	if (vaper != NULL) {
		bcopy((caddr_t)vap, (caddr_t)vaper, sizeof(*vap));
		if (np->n_flag & NCHG) {
			if (np->n_flag & NACC)
				vaper->va_atime = np->n_atim;
			if (np->n_flag & NUPD)
				vaper->va_mtime = np->n_mtim;
		}
	}
	return (0);
}

#ifdef NFS_ACDEBUG
#include <sys/sysctl.h>
SYSCTL_DECL(_vfs_nfs);
static int nfs_acdebug;
SYSCTL_INT(_vfs_nfs, OID_AUTO, acdebug, CTLFLAG_RW, &nfs_acdebug, 0, "");
#endif

/*
 * Check the time stamp
 * If the cache is valid, copy contents to *vap and return 0
 * otherwise return an error
 */
int
nfs_getattrcache(vp, vaper)
	register struct vnode *vp;
	struct vattr *vaper;
{
	register struct nfsnode *np;
	register struct vattr *vap;
	struct nfsmount *nmp;
	int timeo;

	np = VTONFS(vp);
	vap = &np->n_vattr;
	nmp = VFSTONFS(vp->v_mount);
	/* XXX n_mtime doesn't seem to be updated on a miss-and-reload */
	timeo = (time_second - np->n_mtime) / 10;

#ifdef NFS_ACDEBUG
	if (nfs_acdebug>1)
		printf("nfs_getattrcache: initial timeo = %d\n", timeo);
#endif

	if (vap->va_type == VDIR) {
		if ((np->n_flag & NMODIFIED) || timeo < nmp->nm_acdirmin)
			timeo = nmp->nm_acdirmin;
		else if (timeo > nmp->nm_acdirmax)
			timeo = nmp->nm_acdirmax;
	} else {
		if ((np->n_flag & NMODIFIED) || timeo < nmp->nm_acregmin)
			timeo = nmp->nm_acregmin;
		else if (timeo > nmp->nm_acregmax)
			timeo = nmp->nm_acregmax;
	}

#ifdef NFS_ACDEBUG
	if (nfs_acdebug > 2)
		printf("acregmin %d; acregmax %d; acdirmin %d; acdirmax %d\n",
			nmp->nm_acregmin, nmp->nm_acregmax,
			nmp->nm_acdirmin, nmp->nm_acdirmax);

	if (nfs_acdebug)
		printf("nfs_getattrcache: age = %d; final timeo = %d\n",
			(time_second - np->n_attrstamp), timeo);
#endif

	if ((time_second - np->n_attrstamp) >= timeo) {
		nfsstats.attrcache_misses++;
		return (ENOENT);
	}
	nfsstats.attrcache_hits++;
	if (vap->va_size != np->n_size) {
		if (vap->va_type == VREG) {
			if (np->n_flag & NMODIFIED) {
				if (vap->va_size < np->n_size)
					vap->va_size = np->n_size;
				else
					np->n_size = vap->va_size;
			} else {
				np->n_size = vap->va_size;
			}
			vnode_pager_setsize(vp, np->n_size);
		} else {
			np->n_size = vap->va_size;
		}
	}
	bcopy((caddr_t)vap, (caddr_t)vaper, sizeof(struct vattr));
	if (np->n_flag & NCHG) {
		if (np->n_flag & NACC)
			vaper->va_atime = np->n_atim;
		if (np->n_flag & NUPD)
			vaper->va_mtime = np->n_mtim;
	}
	return (0);
}

#ifndef NFS_NOSERVER
/*
 * Set up nameidata for a lookup() call and do it.
 *
 * If pubflag is set, this call is done for a lookup operation on the
 * public filehandle. In that case we allow crossing mountpoints and
 * absolute pathnames. However, the caller is expected to check that
 * the lookup result is within the public fs, and deny access if
 * it is not.
 *
 * nfs_namei() clears out garbage fields that namei() might leave garbage.
 * This is mainly ni_vp and ni_dvp when an error occurs, and ni_dvp when no
 * error occurs but the parent was not requested.
 *
 * dirp may be set whether an error is returned or not, and must be 
 * released by the caller.
 */
int
nfs_namei(ndp, fhp, len, slp, nam, mdp, dposp, retdirp, p, kerbflag, pubflag)
	register struct nameidata *ndp;
	fhandle_t *fhp;
	int len;
	struct nfssvc_sock *slp;
	struct sockaddr *nam;
	struct mbuf **mdp;
	caddr_t *dposp;
	struct vnode **retdirp;
	struct proc *p;
	int kerbflag, pubflag;
{
	register int i, rem;
	register struct mbuf *md;
	register char *fromcp, *tocp, *cp;
	struct iovec aiov;
	struct uio auio;
	struct vnode *dp;
	int error, rdonly, linklen;
	struct componentname *cnp = &ndp->ni_cnd;

	*retdirp = (struct vnode *)0;
	cnp->cn_pnbuf = zalloc(namei_zone);

	/*
	 * Copy the name from the mbuf list to ndp->ni_pnbuf
	 * and set the various ndp fields appropriately.
	 */
	fromcp = *dposp;
	tocp = cnp->cn_pnbuf;
	md = *mdp;
	rem = mtod(md, caddr_t) + md->m_len - fromcp;
	for (i = 0; i < len; i++) {
		while (rem == 0) {
			md = md->m_next;
			if (md == NULL) {
				error = EBADRPC;
				goto out;
			}
			fromcp = mtod(md, caddr_t);
			rem = md->m_len;
		}
		if (*fromcp == '\0' || (!pubflag && *fromcp == '/')) {
			error = EACCES;
			goto out;
		}
		*tocp++ = *fromcp++;
		rem--;
	}
	*tocp = '\0';
	*mdp = md;
	*dposp = fromcp;
	len = nfsm_rndup(len)-len;
	if (len > 0) {
		if (rem >= len)
			*dposp += len;
		else if ((error = nfs_adv(mdp, dposp, len, rem)) != 0)
			goto out;
	}

	/*
	 * Extract and set starting directory.
	 */
	error = nfsrv_fhtovp(fhp, FALSE, &dp, ndp->ni_cnd.cn_cred, slp,
	    nam, &rdonly, kerbflag, pubflag);
	if (error)
		goto out;
	if (dp->v_type != VDIR) {
		vrele(dp);
		error = ENOTDIR;
		goto out;
	}

	if (rdonly)
		cnp->cn_flags |= RDONLY;

	/*
	 * Set return directory.  Reference to dp is implicitly transfered 
	 * to the returned pointer
	 */
	*retdirp = dp;

	if (pubflag) {
		/*
		 * Oh joy. For WebNFS, handle those pesky '%' escapes,
		 * and the 'native path' indicator.
		 */
		cp = zalloc(namei_zone);
		fromcp = cnp->cn_pnbuf;
		tocp = cp;
		if ((unsigned char)*fromcp >= WEBNFS_SPECCHAR_START) {
			switch ((unsigned char)*fromcp) {
			case WEBNFS_NATIVE_CHAR:
				/*
				 * 'Native' path for us is the same
				 * as a path according to the NFS spec,
				 * just skip the escape char.
				 */
				fromcp++;
				break;
			/*
			 * More may be added in the future, range 0x80-0xff
			 */
			default:
				error = EIO;
				zfree(namei_zone, cp);
				goto out;
			}
		}
		/*
		 * Translate the '%' escapes, URL-style.
		 */
		while (*fromcp != '\0') {
			if (*fromcp == WEBNFS_ESC_CHAR) {
				if (fromcp[1] != '\0' && fromcp[2] != '\0') {
					fromcp++;
					*tocp++ = HEXSTRTOI(fromcp);
					fromcp += 2;
					continue;
				} else {
					error = ENOENT;
					zfree(namei_zone, cp);
					goto out;
				}
			} else
				*tocp++ = *fromcp++;
		}
		*tocp = '\0';
		zfree(namei_zone, cnp->cn_pnbuf);
		cnp->cn_pnbuf = cp;
	}

	ndp->ni_pathlen = (tocp - cnp->cn_pnbuf) + 1;
	ndp->ni_segflg = UIO_SYSSPACE;

	if (pubflag) {
		ndp->ni_rootdir = rootvnode;
		ndp->ni_loopcnt = 0;
		if (cnp->cn_pnbuf[0] == '/')
			dp = rootvnode;
	} else {
		cnp->cn_flags |= NOCROSSMOUNT;
	}

	/*
	 * Initialize for scan, set ni_startdir and bump ref on dp again
	 * becuase lookup() will dereference ni_startdir.
	 */

	cnp->cn_proc = p;
	VREF(dp);
	ndp->ni_startdir = dp;

	for (;;) {
		cnp->cn_nameptr = cnp->cn_pnbuf;
		/*
		 * Call lookup() to do the real work.  If an error occurs,
		 * ndp->ni_vp and ni_dvp are left uninitialized or NULL and
		 * we do not have to dereference anything before returning.
		 * In either case ni_startdir will be dereferenced and NULLed
		 * out.
		 */
		error = lookup(ndp);
		if (error)
			break;

		/*
		 * Check for encountering a symbolic link.  Trivial 
		 * termination occurs if no symlink encountered.
		 * Note: zfree is safe because error is 0, so we will
		 * not zfree it again when we break.
		 */
		if ((cnp->cn_flags & ISSYMLINK) == 0) {
			nfsrv_object_create(ndp->ni_vp);
			if (cnp->cn_flags & (SAVENAME | SAVESTART))
				cnp->cn_flags |= HASBUF;
			else
				zfree(namei_zone, cnp->cn_pnbuf);
			break;
		}

		/*
		 * Validate symlink
		 */
		if ((cnp->cn_flags & LOCKPARENT) && ndp->ni_pathlen == 1)
			VOP_UNLOCK(ndp->ni_dvp, 0, p);
		if (!pubflag) {
			error = EINVAL;
			goto badlink2;
		}

		if (ndp->ni_loopcnt++ >= MAXSYMLINKS) {
			error = ELOOP;
			goto badlink2;
		}
		if (ndp->ni_pathlen > 1)
			cp = zalloc(namei_zone);
		else
			cp = cnp->cn_pnbuf;
		aiov.iov_base = cp;
		aiov.iov_len = MAXPATHLEN;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_rw = UIO_READ;
		auio.uio_segflg = UIO_SYSSPACE;
		auio.uio_procp = (struct proc *)0;
		auio.uio_resid = MAXPATHLEN;
		error = VOP_READLINK(ndp->ni_vp, &auio, cnp->cn_cred);
		if (error) {
		badlink1:
			if (ndp->ni_pathlen > 1)
				zfree(namei_zone, cp);
		badlink2:
			vrele(ndp->ni_dvp);
			vput(ndp->ni_vp);
			break;
		}
		linklen = MAXPATHLEN - auio.uio_resid;
		if (linklen == 0) {
			error = ENOENT;
			goto badlink1;
		}
		if (linklen + ndp->ni_pathlen >= MAXPATHLEN) {
			error = ENAMETOOLONG;
			goto badlink1;
		}

		/*
		 * Adjust or replace path
		 */
		if (ndp->ni_pathlen > 1) {
			bcopy(ndp->ni_next, cp + linklen, ndp->ni_pathlen);
			zfree(namei_zone, cnp->cn_pnbuf);
			cnp->cn_pnbuf = cp;
		} else
			cnp->cn_pnbuf[linklen] = '\0';
		ndp->ni_pathlen += linklen;

		/*
		 * Cleanup refs for next loop and check if root directory 
		 * should replace current directory.  Normally ni_dvp 
		 * becomes the new base directory and is cleaned up when
		 * we loop.  Explicitly null pointers after invalidation
		 * to clarify operation.
		 */
		vput(ndp->ni_vp);
		ndp->ni_vp = NULL;

		if (cnp->cn_pnbuf[0] == '/') {
			vrele(ndp->ni_dvp);
			ndp->ni_dvp = ndp->ni_rootdir;
			VREF(ndp->ni_dvp);
		}
		ndp->ni_startdir = ndp->ni_dvp;
		ndp->ni_dvp = NULL;
	}

	/*
	 * nfs_namei() guarentees that fields will not contain garbage
	 * whether an error occurs or not.  This allows the caller to track
	 * cleanup state trivially.
	 */
out:
	if (error) {
		zfree(namei_zone, cnp->cn_pnbuf);
		ndp->ni_vp = NULL;
		ndp->ni_dvp = NULL;
		ndp->ni_startdir = NULL;
		cnp->cn_flags &= ~HASBUF;
	} else if ((ndp->ni_cnd.cn_flags & (WANTPARENT|LOCKPARENT)) == 0) {
		ndp->ni_dvp = NULL;
	}
	return (error);
}

/*
 * A fiddled version of m_adj() that ensures null fill to a long
 * boundary and only trims off the back end
 */
void
nfsm_adj(mp, len, nul)
	struct mbuf *mp;
	register int len;
	int nul;
{
	register struct mbuf *m;
	register int count, i;
	register char *cp;

	/*
	 * Trim from tail.  Scan the mbuf chain,
	 * calculating its length and finding the last mbuf.
	 * If the adjustment only affects this mbuf, then just
	 * adjust and return.  Otherwise, rescan and truncate
	 * after the remaining size.
	 */
	count = 0;
	m = mp;
	for (;;) {
		count += m->m_len;
		if (m->m_next == (struct mbuf *)0)
			break;
		m = m->m_next;
	}
	if (m->m_len > len) {
		m->m_len -= len;
		if (nul > 0) {
			cp = mtod(m, caddr_t)+m->m_len-nul;
			for (i = 0; i < nul; i++)
				*cp++ = '\0';
		}
		return;
	}
	count -= len;
	if (count < 0)
		count = 0;
	/*
	 * Correct length for chain is "count".
	 * Find the mbuf with last data, adjust its length,
	 * and toss data from remaining mbufs on chain.
	 */
	for (m = mp; m; m = m->m_next) {
		if (m->m_len >= count) {
			m->m_len = count;
			if (nul > 0) {
				cp = mtod(m, caddr_t)+m->m_len-nul;
				for (i = 0; i < nul; i++)
					*cp++ = '\0';
			}
			break;
		}
		count -= m->m_len;
	}
	for (m = m->m_next;m;m = m->m_next)
		m->m_len = 0;
}

/*
 * Make these functions instead of macros, so that the kernel text size
 * doesn't get too big...
 */
void
nfsm_srvwcc(nfsd, before_ret, before_vap, after_ret, after_vap, mbp, bposp)
	struct nfsrv_descript *nfsd;
	int before_ret;
	register struct vattr *before_vap;
	int after_ret;
	struct vattr *after_vap;
	struct mbuf **mbp;
	char **bposp;
{
	register struct mbuf *mb = *mbp, *mb2;
	register char *bpos = *bposp;
	register u_int32_t *tl;

	if (before_ret) {
		nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED);
		*tl = nfs_false;
	} else {
		nfsm_build(tl, u_int32_t *, 7 * NFSX_UNSIGNED);
		*tl++ = nfs_true;
		txdr_hyper(before_vap->va_size, tl);
		tl += 2;
		txdr_nfsv3time(&(before_vap->va_mtime), tl);
		tl += 2;
		txdr_nfsv3time(&(before_vap->va_ctime), tl);
	}
	*bposp = bpos;
	*mbp = mb;
	nfsm_srvpostopattr(nfsd, after_ret, after_vap, mbp, bposp);
}

void
nfsm_srvpostopattr(nfsd, after_ret, after_vap, mbp, bposp)
	struct nfsrv_descript *nfsd;
	int after_ret;
	struct vattr *after_vap;
	struct mbuf **mbp;
	char **bposp;
{
	register struct mbuf *mb = *mbp, *mb2;
	register char *bpos = *bposp;
	register u_int32_t *tl;
	register struct nfs_fattr *fp;

	if (after_ret) {
		nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED);
		*tl = nfs_false;
	} else {
		nfsm_build(tl, u_int32_t *, NFSX_UNSIGNED + NFSX_V3FATTR);
		*tl++ = nfs_true;
		fp = (struct nfs_fattr *)tl;
		nfsm_srvfattr(nfsd, after_vap, fp);
	}
	*mbp = mb;
	*bposp = bpos;
}

void
nfsm_srvfattr(nfsd, vap, fp)
	register struct nfsrv_descript *nfsd;
	register struct vattr *vap;
	register struct nfs_fattr *fp;
{

	fp->fa_nlink = txdr_unsigned(vap->va_nlink);
	fp->fa_uid = txdr_unsigned(vap->va_uid);
	fp->fa_gid = txdr_unsigned(vap->va_gid);
	if (nfsd->nd_flag & ND_NFSV3) {
		fp->fa_type = vtonfsv3_type(vap->va_type);
		fp->fa_mode = vtonfsv3_mode(vap->va_mode);
		txdr_hyper(vap->va_size, &fp->fa3_size);
		txdr_hyper(vap->va_bytes, &fp->fa3_used);
		fp->fa3_rdev.specdata1 = txdr_unsigned(umajor(vap->va_rdev));
		fp->fa3_rdev.specdata2 = txdr_unsigned(uminor(vap->va_rdev));
		fp->fa3_fsid.nfsuquad[0] = 0;
		fp->fa3_fsid.nfsuquad[1] = txdr_unsigned(vap->va_fsid);
		fp->fa3_fileid.nfsuquad[0] = 0;
		fp->fa3_fileid.nfsuquad[1] = txdr_unsigned(vap->va_fileid);
		txdr_nfsv3time(&vap->va_atime, &fp->fa3_atime);
		txdr_nfsv3time(&vap->va_mtime, &fp->fa3_mtime);
		txdr_nfsv3time(&vap->va_ctime, &fp->fa3_ctime);
	} else {
		fp->fa_type = vtonfsv2_type(vap->va_type);
		fp->fa_mode = vtonfsv2_mode(vap->va_type, vap->va_mode);
		fp->fa2_size = txdr_unsigned(vap->va_size);
		fp->fa2_blocksize = txdr_unsigned(vap->va_blocksize);
		if (vap->va_type == VFIFO)
			fp->fa2_rdev = 0xffffffff;
		else
			fp->fa2_rdev = txdr_unsigned(vap->va_rdev);
		fp->fa2_blocks = txdr_unsigned(vap->va_bytes / NFS_FABLKSIZE);
		fp->fa2_fsid = txdr_unsigned(vap->va_fsid);
		fp->fa2_fileid = txdr_unsigned(vap->va_fileid);
		txdr_nfsv2time(&vap->va_atime, &fp->fa2_atime);
		txdr_nfsv2time(&vap->va_mtime, &fp->fa2_mtime);
		txdr_nfsv2time(&vap->va_ctime, &fp->fa2_ctime);
	}
}

/*
 * nfsrv_fhtovp() - convert a fh to a vnode ptr (optionally locked)
 * 	- look up fsid in mount list (if not found ret error)
 *	- get vp and export rights by calling VFS_FHTOVP()
 *	- if cred->cr_uid == 0 or MNT_EXPORTANON set it to credanon
 *	- if not lockflag unlock it with VOP_UNLOCK()
 */
int
nfsrv_fhtovp(fhp, lockflag, vpp, cred, slp, nam, rdonlyp, kerbflag, pubflag)
	fhandle_t *fhp;
	int lockflag;
	struct vnode **vpp;
	struct ucred *cred;
	struct nfssvc_sock *slp;
	struct sockaddr *nam;
	int *rdonlyp;
	int kerbflag;
	int pubflag;
{
	struct proc *p = curproc; /* XXX */
	register struct mount *mp;
	register int i;
	struct ucred *credanon;
	int error, exflags;
#ifdef MNT_EXNORESPORT		/* XXX needs mountd and /etc/exports help yet */
	struct sockaddr_int *saddr;
#endif

	*vpp = (struct vnode *)0;

	if (nfs_ispublicfh(fhp)) {
		if (!pubflag || !nfs_pub.np_valid)
			return (ESTALE);
		fhp = &nfs_pub.np_handle;
	}

	mp = vfs_getvfs(&fhp->fh_fsid);
	if (!mp)
		return (ESTALE);
	error = VFS_CHECKEXP(mp, nam, &exflags, &credanon);
	if (error)
		return (error); 
	error = VFS_FHTOVP(mp, &fhp->fh_fid, vpp);
	if (error)
		return (error);
#ifdef MNT_EXNORESPORT
	if (!(exflags & (MNT_EXNORESPORT|MNT_EXPUBLIC))) {
		saddr = (struct sockaddr_in *)nam;
		if (saddr->sin_family == AF_INET &&
		    ntohs(saddr->sin_port) >= IPPORT_RESERVED) {
			vput(*vpp);
			*vpp = NULL;
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
	}
#endif
	/*
	 * Check/setup credentials.
	 */
	if (exflags & MNT_EXKERB) {
		if (!kerbflag) {
			vput(*vpp);
			*vpp = NULL;
			return (NFSERR_AUTHERR | AUTH_TOOWEAK);
		}
	} else if (kerbflag) {
		vput(*vpp);
		*vpp = NULL;
		return (NFSERR_AUTHERR | AUTH_TOOWEAK);
	} else if (cred->cr_uid == 0 || (exflags & MNT_EXPORTANON)) {
		cred->cr_uid = credanon->cr_uid;
		for (i = 0; i < credanon->cr_ngroups && i < NGROUPS; i++)
			cred->cr_groups[i] = credanon->cr_groups[i];
		cred->cr_ngroups = i;
	}
	if (exflags & MNT_EXRDONLY)
		*rdonlyp = 1;
	else
		*rdonlyp = 0;

	nfsrv_object_create(*vpp);

	if (!lockflag)
		VOP_UNLOCK(*vpp, 0, p);
	return (0);
}


/*
 * WebNFS: check if a filehandle is a public filehandle. For v3, this
 * means a length of 0, for v2 it means all zeroes. nfsm_srvmtofh has
 * transformed this to all zeroes in both cases, so check for it.
 */
int
nfs_ispublicfh(fhp)
	fhandle_t *fhp;
{
	char *cp = (char *)fhp;
	int i;

	for (i = 0; i < NFSX_V3FH; i++)
		if (*cp++ != 0)
			return (FALSE);
	return (TRUE);
}
  
#endif /* NFS_NOSERVER */
/*
 * This function compares two net addresses by family and returns TRUE
 * if they are the same host.
 * If there is any doubt, return FALSE.
 * The AF_INET family is handled as a special case so that address mbufs
 * don't need to be saved to store "struct in_addr", which is only 4 bytes.
 */
int
netaddr_match(family, haddr, nam)
	int family;
	union nethostaddr *haddr;
	struct sockaddr *nam;
{
	register struct sockaddr_in *inetaddr;

	switch (family) {
	case AF_INET:
		inetaddr = (struct sockaddr_in *)nam;
		if (inetaddr->sin_family == AF_INET &&
		    inetaddr->sin_addr.s_addr == haddr->had_inetaddr)
			return (1);
		break;
	default:
		break;
	};
	return (0);
}

static nfsuint64 nfs_nullcookie = { { 0, 0 } };
/*
 * This function finds the directory cookie that corresponds to the
 * logical byte offset given.
 */
nfsuint64 *
nfs_getcookie(np, off, add)
	register struct nfsnode *np;
	off_t off;
	int add;
{
	register struct nfsdmap *dp, *dp2;
	register int pos;

	pos = (uoff_t)off / NFS_DIRBLKSIZ;
	if (pos == 0 || off < 0) {
#ifdef DIAGNOSTIC
		if (add)
			panic("nfs getcookie add at <= 0");
#endif
		return (&nfs_nullcookie);
	}
	pos--;
	dp = np->n_cookies.lh_first;
	if (!dp) {
		if (add) {
			MALLOC(dp, struct nfsdmap *, sizeof (struct nfsdmap),
				M_NFSDIROFF, M_WAITOK);
			dp->ndm_eocookie = 0;
			LIST_INSERT_HEAD(&np->n_cookies, dp, ndm_list);
		} else
			return ((nfsuint64 *)0);
	}
	while (pos >= NFSNUMCOOKIES) {
		pos -= NFSNUMCOOKIES;
		if (dp->ndm_list.le_next) {
			if (!add && dp->ndm_eocookie < NFSNUMCOOKIES &&
				pos >= dp->ndm_eocookie)
				return ((nfsuint64 *)0);
			dp = dp->ndm_list.le_next;
		} else if (add) {
			MALLOC(dp2, struct nfsdmap *, sizeof (struct nfsdmap),
				M_NFSDIROFF, M_WAITOK);
			dp2->ndm_eocookie = 0;
			LIST_INSERT_AFTER(dp, dp2, ndm_list);
			dp = dp2;
		} else
			return ((nfsuint64 *)0);
	}
	if (pos >= dp->ndm_eocookie) {
		if (add)
			dp->ndm_eocookie = pos + 1;
		else
			return ((nfsuint64 *)0);
	}
	return (&dp->ndm_cookies[pos]);
}

/*
 * Invalidate cached directory information, except for the actual directory
 * blocks (which are invalidated separately).
 * Done mainly to avoid the use of stale offset cookies.
 */
void
nfs_invaldir(vp)
	register struct vnode *vp;
{
	register struct nfsnode *np = VTONFS(vp);

#ifdef DIAGNOSTIC
	if (vp->v_type != VDIR)
		panic("nfs: invaldir not dir");
#endif
	np->n_direofoffset = 0;
	np->n_cookieverf.nfsuquad[0] = 0;
	np->n_cookieverf.nfsuquad[1] = 0;
	if (np->n_cookies.lh_first)
		np->n_cookies.lh_first->ndm_eocookie = 0;
}

/*
 * The write verifier has changed (probably due to a server reboot), so all
 * B_NEEDCOMMIT blocks will have to be written again. Since they are on the
 * dirty block list as B_DELWRI, all this takes is clearing the B_NEEDCOMMIT
 * and B_CLUSTEROK flags.  Once done the new write verifier can be set for the
 * mount point.
 *
 * B_CLUSTEROK must be cleared along with B_NEEDCOMMIT because stage 1 data 
 * writes are not clusterable.
 */
void
nfs_clearcommit(mp)
	struct mount *mp;
{
	register struct vnode *vp, *nvp;
	register struct buf *bp, *nbp;
	int s;

	s = splbio();
	mtx_assert(&Giant, MA_OWNED);
	mtx_assert(&vm_mtx, MA_NOTOWNED);
	mtx_lock(&mntvnode_mtx);
loop:
	for (vp = LIST_FIRST(&mp->mnt_vnodelist); vp; vp = nvp) {
		if (vp->v_mount != mp)	/* Paranoia */
			goto loop;
		nvp = LIST_NEXT(vp, v_mntvnodes);
		for (bp = TAILQ_FIRST(&vp->v_dirtyblkhd); bp; bp = nbp) {
			nbp = TAILQ_NEXT(bp, b_vnbufs);
			if (BUF_REFCNT(bp) == 0 &&
			    (bp->b_flags & (B_DELWRI | B_NEEDCOMMIT))
				== (B_DELWRI | B_NEEDCOMMIT))
				bp->b_flags &= ~(B_NEEDCOMMIT | B_CLUSTEROK);
		}
	}
	mtx_unlock(&mntvnode_mtx);
	splx(s);
}

#ifndef NFS_NOSERVER
/*
 * Map errnos to NFS error numbers. For Version 3 also filter out error
 * numbers not specified for the associated procedure.
 */
int
nfsrv_errmap(nd, err)
	struct nfsrv_descript *nd;
	register int err;
{
	register short *defaulterrp, *errp;

	if (nd->nd_flag & ND_NFSV3) {
	    if (nd->nd_procnum <= NFSPROC_COMMIT) {
		errp = defaulterrp = nfsrv_v3errmap[nd->nd_procnum];
		while (*++errp) {
			if (*errp == err)
				return (err);
			else if (*errp > err)
				break;
		}
		return ((int)*defaulterrp);
	    } else
		return (err & 0xffff);
	}
	if (err <= ELAST)
		return ((int)nfsrv_v2errmap[err - 1]);
	return (NFSERR_IO);
}

int
nfsrv_object_create(vp)
	struct vnode *vp;
{

	if (vp == NULL || vp->v_type != VREG)
		return (1);
	return (vfs_object_create(vp, curproc,
				  curproc ? curproc->p_ucred : NULL));
}

/*
 * Sort the group list in increasing numerical order.
 * (Insertion sort by Chris Torek, who was grossed out by the bubble sort
 *  that used to be here.)
 */
void
nfsrvw_sort(list, num)
        register gid_t *list;
        register int num;
{
	register int i, j;
	gid_t v;

	/* Insertion sort. */
	for (i = 1; i < num; i++) {
		v = list[i];
		/* find correct slot for value v, moving others up */
		for (j = i; --j >= 0 && v < list[j];)
			list[j + 1] = list[j];
		list[j + 1] = v;
	}
}

/*
 * copy credentials making sure that the result can be compared with bcmp().
 */
void
nfsrv_setcred(incred, outcred)
	register struct ucred *incred, *outcred;
{
	register int i;

	bzero((caddr_t)outcred, sizeof (struct ucred));
	outcred->cr_ref = 1;
	outcred->cr_uid = incred->cr_uid;
	outcred->cr_ngroups = incred->cr_ngroups;
	for (i = 0; i < incred->cr_ngroups; i++)
		outcred->cr_groups[i] = incred->cr_groups[i];
	nfsrvw_sort(outcred->cr_groups, outcred->cr_ngroups);
}
#endif /* NFS_NOSERVER */

.\" Copyright (c) 1993
.\"	The Regents of the University of California.  All rights reserved.
.\"
.\" This document is derived from software contributed to Berkeley by
.\" Rick Macklem at The University of Guelph.
.\"
.\" %sccs.include.redist.roff%
.\"
.\"	@(#)0.t	8.1 (Berkeley) %G%
.\"
.(l C
.sz 14
.b "The 4.4BSD NFS Implementation"
.sp
.sz 10
Rick Macklem
.i "University of Guelph"
.)l
.sp 2
.ce 1
.sz 12
.b "ABSTRACT"
.eh 'SMM:06-%''The 4.4BSD NFS Implementation'
.oh 'The 4.4BSD NFS Implementation''SMM:06-%'
.pp
The 4.4BSD implementation of the Network File System (NFS)\** is
intended to interoperate with
.(f
\**Network File System (NFS) is believed to be a registered trademark of
Sun Microsystems Inc.
.)f
other NFS Version 2 Protocol (RFC1094) implementations but also
allows use of an alternate protocol that is hoped to provide better
performance in certain environments.
This paper will informally discuss these various protocol features and
their use.
There is a brief overview of the implementation followed
by several sections on various problem areas related to NFS
and some hints on how to deal with them.
.pp
Not Quite NFS (NQNFS) is an NFS like protocol designed to maintain full cache
consistency between clients in a crash tolerant manner. It is an adaptation
of the NFS protocol such that the server supports both NFS
and NQNFS clients while maintaining full consistency between the server and
NQNFS clients.
It borrows heavily from work done on Spritely-NFS [Srinivasan89], but uses
Leases [Gray89] to avoid the need to recover server state information
after a crash.
.sp

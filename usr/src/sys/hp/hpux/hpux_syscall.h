/*
 * System call numbers.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	@(#)syscalls.master	8.1 (Berkeley) 6/14/93
 */

#define	SYS_exit	1
#define	SYS_fork	2
#define	SYS_read	3
#define	SYS_write	4
#define	SYS_open	5
#define	SYS_close	6
#define	SYS_owait	7
#define	SYS_ocreat	8
#define	SYS_link	9
#define	SYS_unlink	10
#define	SYS_execv	11
#define	SYS_chdir	12
				/* 13 is old hpuxtime time */
#define	SYS_mknod	14
#define	SYS_chmod	15
#define	SYS_chown	16
#define	SYS_break	17
				/* 18 is old hpuxstat stat */
#define	SYS_olseek	19
#define	SYS_getpid	20
#define	SYS_setuid	23
#define	SYS_getuid	24
				/* 25 is old hpuxstime stime */
#define	SYS_ptrace	26
				/* 27 is old hpuxalarm alarm */
				/* 28 is old hpuxfstat fstat */
				/* 29 is old hpuxpause pause */
				/* 30 is old hpuxutime utime */
				/* 31 is old hpuxstty stty */
				/* 32 is old hpuxgtty gtty */
#define	SYS_access	33
				/* 34 is old hpuxnice nice */
				/* 35 is old hpuxftime ftime */
#define	SYS_sync	36
#define	SYS_kill	37
#define	SYS_stat	38
				/* 39 is old hpuxsetpgrp setpgrp */
#define	SYS_lstat	40
#define	SYS_dup	41
#define	SYS_pipe	42
				/* 43 is old hpuxtimes times */
#define	SYS_profil	44
#define	SYS_setgid	46
#define	SYS_getgid	47
				/* 48 is old hpuxssig ssig */
#define	SYS_ioctl	54
#define	SYS_symlink	56
#define	SYS_utssys	57
#define	SYS_readlink	58
#define	SYS_execve	59
#define	SYS_umask	60
#define	SYS_chroot	61
#define	SYS_fcntl	62
#define	SYS_ulimit	63
#define	SYS_vfork	66
#define	SYS_vread	67
#define	SYS_vwrite	68
#define	SYS_mmap	71
#define	SYS_getgroups	79
#define	SYS_setgroups	80
#define	SYS_getpgrp2	81
#define	SYS_setpgrp2	82
#define	SYS_setitimer	83
#define	SYS_wait3	84
#define	SYS_getitimer	86
#define	SYS_dup2	90
#define	SYS_fstat	92
#define	SYS_select	93
#define	SYS_fsync	95
#define	SYS_sigreturn	103
#define	SYS_sigvec	108
#define	SYS_sigblock	109
#define	SYS_sigsetmask	110
#define	SYS_sigpause	111
#define	SYS_osigstack	112
#define	SYS_gettimeofday	116
#define	SYS_readv	120
#define	SYS_writev	121
#define	SYS_settimeofday	122
#define	SYS_fchown	123
#define	SYS_fchmod	124
#define	SYS_setresuid	126
#define	SYS_setresgid	127
#define	SYS_rename	128
				/* 129 is old truncate */
				/* 130 is old ftruncate */
#define	SYS_sysconf	132
#define	SYS_mkdir	136
#define	SYS_rmdir	137
				/* 144 is old getrlimit */
				/* 145 is old setrlimit */
#define	SYS_rtprio	152
#define	SYS_netioctl	154
#define	SYS_lockf	155
#define	SYS_semget	156
#define	SYS_semctl	157
#define	SYS_semop	158
#define	SYS_shmget	163
#define	SYS_shmctl	164
#define	SYS_shmat	165
#define	SYS_shmdt	166
#define	SYS_m68020_advise	167
#define	SYS_getcontext	174
#define	SYS_getaccess	190
#define	SYS_waitpid	200
#define	SYS_getdirentries	231
#define	SYS_getdomainname	232
#define	SYS_setdomainname	236
#define	SYS_sigaction	239
#define	SYS_sigprocmask	240
#define	SYS_sigpending	241
#define	SYS_sigsuspend	242
#define	SYS_getnumfds	268
				/* 275 is old accept */
#define	SYS_bind	276
#define	SYS_connect	277
				/* 278 is old getpeername */
				/* 279 is old getsockname */
#define	SYS_getsockopt	280
#define	SYS_listen	281
				/* 282 is old recv */
				/* 283 is old recvfrom */
				/* 284 is old recvmsg */
				/* 285 is old send */
				/* 286 is old sendmsg */
#define	SYS_sendto	287
#define	SYS_setsockopt2	288
#define	SYS_shutdown	289
#define	SYS_socket	290
#define	SYS_socketpair	291

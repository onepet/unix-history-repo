/*
 * System call prototypes.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	Id: syscalls.master,v 1.2 1996/03/04 20:58:47 peter Exp 
 */

#ifndef _LINUX_SYSPROTO_H_
#define	_LINUX_SYSPROTO_H_

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>

struct proc;

#define	PAD_(t) (sizeof(register_t) <= sizeof(t) ? \
		0 : sizeof(register_t) - sizeof(t))

struct	linux_setup_args {
	register_t dummy;
};
struct	linux_fork_args {
	register_t dummy;
};
struct	linux_open_args {
	char *	path;	char path_[PAD_(char *)];
	int	flags;	char flags_[PAD_(int)];
	int	mode;	char mode_[PAD_(int)];
};
struct	linux_waitpid_args {
	int	pid;	char pid_[PAD_(int)];
	int *	status;	char status_[PAD_(int *)];
	int	options;	char options_[PAD_(int)];
};
struct	linux_creat_args {
	char *	path;	char path_[PAD_(char *)];
	int	mode;	char mode_[PAD_(int)];
};
struct	linux_unlink_args {
	char *	path;	char path_[PAD_(char *)];
};
struct	linux_execve_args {
	char *	path;	char path_[PAD_(char *)];
	char **	argp;	char argp_[PAD_(char **)];
	char **	envp;	char envp_[PAD_(char **)];
};
struct	linux_chdir_args {
	char *	path;	char path_[PAD_(char *)];
};
struct	linux_time_args {
	linux_time_t *	tm;	char tm_[PAD_(linux_time_t *)];
};
struct	linux_mknod_args {
	char *	path;	char path_[PAD_(char *)];
	int	mode;	char mode_[PAD_(int)];
	int	dev;	char dev_[PAD_(int)];
};
struct	linux_chmod_args {
	char *	path;	char path_[PAD_(char *)];
	int	mode;	char mode_[PAD_(int)];
};
struct	linux_lchown_args {
	char *	path;	char path_[PAD_(char *)];
	int	uid;	char uid_[PAD_(int)];
	int	gid;	char gid_[PAD_(int)];
};
struct	linux_break_args {
	char *	nsize;	char nsize_[PAD_(char *)];
};
struct	linux_stat_args {
	char *	path;	char path_[PAD_(char *)];
	struct ostat *	up;	char up_[PAD_(struct ostat *)];
};
struct	linux_lseek_args {
	int	fdes;	char fdes_[PAD_(int)];
	long	off;	char off_[PAD_(long)];
	int	whence;	char whence_[PAD_(int)];
};
struct	linux_mount_args {
	register_t dummy;
};
struct	linux_umount_args {
	register_t dummy;
};
struct	linux_stime_args {
	register_t dummy;
};
struct	linux_ptrace_args {
	register_t dummy;
};
struct	linux_alarm_args {
	unsigned int	secs;	char secs_[PAD_(unsigned int)];
};
struct	linux_fstat_args {
	int	fd;	char fd_[PAD_(int)];
	struct ostat *	up;	char up_[PAD_(struct ostat *)];
};
struct	linux_pause_args {
	register_t dummy;
};
struct	linux_utime_args {
	char *	fname;	char fname_[PAD_(char *)];
	struct linux_utimbuf *	times;	char times_[PAD_(struct linux_utimbuf *)];
};
struct	linux_stty_args {
	register_t dummy;
};
struct	linux_gtty_args {
	register_t dummy;
};
struct	linux_access_args {
	char *	path;	char path_[PAD_(char *)];
	int	flags;	char flags_[PAD_(int)];
};
struct	linux_nice_args {
	int	inc;	char inc_[PAD_(int)];
};
struct	linux_ftime_args {
	register_t dummy;
};
struct	linux_kill_args {
	int	pid;	char pid_[PAD_(int)];
	int	signum;	char signum_[PAD_(int)];
};
struct	linux_rename_args {
	char *	from;	char from_[PAD_(char *)];
	char *	to;	char to_[PAD_(char *)];
};
struct	linux_mkdir_args {
	char *	path;	char path_[PAD_(char *)];
	int	mode;	char mode_[PAD_(int)];
};
struct	linux_rmdir_args {
	char *	path;	char path_[PAD_(char *)];
};
struct	linux_pipe_args {
	int *	pipefds;	char pipefds_[PAD_(int *)];
};
struct	linux_times_args {
	struct linux_times_argv *	buf;	char buf_[PAD_(struct linux_times_argv *)];
};
struct	linux_prof_args {
	register_t dummy;
};
struct	linux_brk_args {
	char *	dsend;	char dsend_[PAD_(char *)];
};
struct	linux_signal_args {
	int	sig;	char sig_[PAD_(int)];
	linux_handler_t	handler;	char handler_[PAD_(linux_handler_t)];
};
struct	linux_phys_args {
	register_t dummy;
};
struct	linux_lock_args {
	register_t dummy;
};
struct	linux_ioctl_args {
	int	fd;	char fd_[PAD_(int)];
	u_long	cmd;	char cmd_[PAD_(u_long)];
	int	arg;	char arg_[PAD_(int)];
};
struct	linux_fcntl_args {
	int	fd;	char fd_[PAD_(int)];
	int	cmd;	char cmd_[PAD_(int)];
	int	arg;	char arg_[PAD_(int)];
};
struct	linux_mpx_args {
	register_t dummy;
};
struct	linux_ulimit_args {
	register_t dummy;
};
struct	linux_olduname_args {
	struct linux_oldold_utsname *	up;	char up_[PAD_(struct linux_oldold_utsname *)];
};
struct	linux_ustat_args {
	register_t dummy;
};
struct	linux_sigaction_args {
	int	sig;	char sig_[PAD_(int)];
	struct linux_sigaction *	nsa;	char nsa_[PAD_(struct linux_sigaction *)];
	struct linux_sigaction *	osa;	char osa_[PAD_(struct linux_sigaction *)];
};
struct	linux_siggetmask_args {
	register_t dummy;
};
struct	linux_sigsetmask_args {
	linux_sigset_t	mask;	char mask_[PAD_(linux_sigset_t)];
};
struct	linux_sigsuspend_args {
	int	restart;	char restart_[PAD_(int)];
	linux_sigset_t	oldmask;	char oldmask_[PAD_(linux_sigset_t)];
	linux_sigset_t	mask;	char mask_[PAD_(linux_sigset_t)];
};
struct	linux_sigpending_args {
	linux_sigset_t *	mask;	char mask_[PAD_(linux_sigset_t *)];
};
struct	linux_select_args {
	struct linux_select_argv *	ptr;	char ptr_[PAD_(struct linux_select_argv *)];
};
struct	linux_symlink_args {
	char *	path;	char path_[PAD_(char *)];
	char *	to;	char to_[PAD_(char *)];
};
struct	linux_readlink_args {
	char *	name;	char name_[PAD_(char *)];
	char *	buf;	char buf_[PAD_(char *)];
	int	count;	char count_[PAD_(int)];
};
struct	linux_uselib_args {
	char *	library;	char library_[PAD_(char *)];
};
struct	linux_readdir_args {
	int	fd;	char fd_[PAD_(int)];
	struct linux_dirent *	dent;	char dent_[PAD_(struct linux_dirent *)];
	unsigned int	count;	char count_[PAD_(unsigned int)];
};
struct	linux_mmap_args {
	struct linux_mmap_argv *	ptr;	char ptr_[PAD_(struct linux_mmap_argv *)];
};
struct	linux_truncate_args {
	char *	path;	char path_[PAD_(char *)];
	long	length;	char length_[PAD_(long)];
};
struct	linux_statfs_args {
	char *	path;	char path_[PAD_(char *)];
	struct linux_statfs_buf *	buf;	char buf_[PAD_(struct linux_statfs_buf *)];
};
struct	linux_fstatfs_args {
	int	fd;	char fd_[PAD_(int)];
	struct linux_statfs_buf *	buf;	char buf_[PAD_(struct linux_statfs_buf *)];
};
struct	linux_ioperm_args {
	unsigned int	lo;	char lo_[PAD_(unsigned int)];
	unsigned int	hi;	char hi_[PAD_(unsigned int)];
	int	val;	char val_[PAD_(int)];
};
struct	linux_socketcall_args {
	int	what;	char what_[PAD_(int)];
	void *	args;	char args_[PAD_(void *)];
};
struct	linux_ksyslog_args {
	int	what;	char what_[PAD_(int)];
};
struct	linux_setitimer_args {
	u_int	which;	char which_[PAD_(u_int)];
	struct itimerval *	itv;	char itv_[PAD_(struct itimerval *)];
	struct itimerval *	oitv;	char oitv_[PAD_(struct itimerval *)];
};
struct	linux_getitimer_args {
	u_int	which;	char which_[PAD_(u_int)];
	struct itimerval *	itv;	char itv_[PAD_(struct itimerval *)];
};
struct	linux_newstat_args {
	char *	path;	char path_[PAD_(char *)];
	struct linux_newstat *	buf;	char buf_[PAD_(struct linux_newstat *)];
};
struct	linux_newlstat_args {
	char *	path;	char path_[PAD_(char *)];
	struct linux_newstat *	buf;	char buf_[PAD_(struct linux_newstat *)];
};
struct	linux_newfstat_args {
	int	fd;	char fd_[PAD_(int)];
	struct linux_newstat *	buf;	char buf_[PAD_(struct linux_newstat *)];
};
struct	linux_uname_args {
	struct linux_old_utsname *	up;	char up_[PAD_(struct linux_old_utsname *)];
};
struct	linux_iopl_args {
	int	level;	char level_[PAD_(int)];
};
struct	linux_vhangup_args {
	register_t dummy;
};
struct	linux_idle_args {
	register_t dummy;
};
struct	linux_vm86_args {
	register_t dummy;
};
struct	linux_wait4_args {
	int	pid;	char pid_[PAD_(int)];
	int *	status;	char status_[PAD_(int *)];
	int	options;	char options_[PAD_(int)];
	struct rusage *	rusage;	char rusage_[PAD_(struct rusage *)];
};
struct	linux_swapoff_args {
	register_t dummy;
};
struct	linux_sysinfo_args {
	register_t dummy;
};
struct	linux_ipc_args {
	int	what;	char what_[PAD_(int)];
	int	arg1;	char arg1_[PAD_(int)];
	int	arg2;	char arg2_[PAD_(int)];
	int	arg3;	char arg3_[PAD_(int)];
	caddr_t	ptr;	char ptr_[PAD_(caddr_t)];
};
struct	linux_sigreturn_args {
	struct linux_sigcontext *	scp;	char scp_[PAD_(struct linux_sigcontext *)];
};
struct	linux_clone_args {
	register_t dummy;
};
struct	linux_newuname_args {
	struct linux_newuname_t *	buf;	char buf_[PAD_(struct linux_newuname_t *)];
};
struct	linux_modify_ldt_args {
	int	func;	char func_[PAD_(int)];
	void *	ptr;	char ptr_[PAD_(void *)];
	size_t	bytecount;	char bytecount_[PAD_(size_t)];
};
struct	linux_adjtimex_args {
	register_t dummy;
};
struct	linux_sigprocmask_args {
	int	how;	char how_[PAD_(int)];
	linux_sigset_t *	mask;	char mask_[PAD_(linux_sigset_t *)];
	linux_sigset_t *	omask;	char omask_[PAD_(linux_sigset_t *)];
};
struct	linux_create_module_args {
	register_t dummy;
};
struct	linux_init_module_args {
	register_t dummy;
};
struct	linux_delete_module_args {
	register_t dummy;
};
struct	linux_get_kernel_syms_args {
	register_t dummy;
};
struct	linux_quotactl_args {
	register_t dummy;
};
struct	linux_getpgid_args {
	int	pid;	char pid_[PAD_(int)];
};
struct	linux_bdflush_args {
	register_t dummy;
};
struct	linux_personality_args {
	int	per;	char per_[PAD_(int)];
};
struct	linux_llseek_args {
	int	fd;	char fd_[PAD_(int)];
	u_int32_t	ohigh;	char ohigh_[PAD_(u_int32_t)];
	u_int32_t	olow;	char olow_[PAD_(u_int32_t)];
	caddr_t	res;	char res_[PAD_(caddr_t)];
	int	whence;	char whence_[PAD_(int)];
};
struct	linux_getdents_args {
	int	fd;	char fd_[PAD_(int)];
	void *	dent;	char dent_[PAD_(void *)];
	unsigned	count;	char count_[PAD_(unsigned)];
};
struct	linux_newselect_args {
	int	nfds;	char nfds_[PAD_(int)];
	fd_set *	readfds;	char readfds_[PAD_(fd_set *)];
	fd_set *	writefds;	char writefds_[PAD_(fd_set *)];
	fd_set *	exceptfds;	char exceptfds_[PAD_(fd_set *)];
	struct timeval *	timeout;	char timeout_[PAD_(struct timeval *)];
};
struct	linux_msync_args {
	caddr_t	addr;	char addr_[PAD_(caddr_t)];
	int	len;	char len_[PAD_(int)];
	int	fl;	char fl_[PAD_(int)];
};
struct	linux_mremap_args {
	caddr_t	addr;	char addr_[PAD_(caddr_t)];
	int	old_len;	char old_len_[PAD_(int)];
	int	new_len;	char new_len_[PAD_(int)];
	int	flags;	char flags_[PAD_(int)];
};
struct	linux_chown_args {
	char *	path;	char path_[PAD_(char *)];
	int	uid;	char uid_[PAD_(int)];
	int	gid;	char gid_[PAD_(int)];
};
int	linux_setup __P((struct proc *, struct linux_setup_args *, int []));
int	linux_fork __P((struct proc *, struct linux_fork_args *, int []));
int	linux_open __P((struct proc *, struct linux_open_args *, int []));
int	linux_waitpid __P((struct proc *, struct linux_waitpid_args *, int []));
int	linux_creat __P((struct proc *, struct linux_creat_args *, int []));
int	linux_unlink __P((struct proc *, struct linux_unlink_args *, int []));
int	linux_execve __P((struct proc *, struct linux_execve_args *, int []));
int	linux_chdir __P((struct proc *, struct linux_chdir_args *, int []));
int	linux_time __P((struct proc *, struct linux_time_args *, int []));
int	linux_mknod __P((struct proc *, struct linux_mknod_args *, int []));
int	linux_chmod __P((struct proc *, struct linux_chmod_args *, int []));
int	linux_chown __P((struct proc *, struct linux_chown_args *, int []));
int	linux_lchown __P((struct proc *, struct linux_lchown_args *, int []));
int	linux_break __P((struct proc *, struct linux_break_args *, int []));
int	linux_stat __P((struct proc *, struct linux_stat_args *, int []));
int	linux_lseek __P((struct proc *, struct linux_lseek_args *, int []));
int	linux_mount __P((struct proc *, struct linux_mount_args *, int []));
int	linux_umount __P((struct proc *, struct linux_umount_args *, int []));
int	linux_stime __P((struct proc *, struct linux_stime_args *, int []));
int	linux_ptrace __P((struct proc *, struct linux_ptrace_args *, int []));
int	linux_alarm __P((struct proc *, struct linux_alarm_args *, int []));
int	linux_fstat __P((struct proc *, struct linux_fstat_args *, int []));
int	linux_pause __P((struct proc *, struct linux_pause_args *, int []));
int	linux_utime __P((struct proc *, struct linux_utime_args *, int []));
int	linux_stty __P((struct proc *, struct linux_stty_args *, int []));
int	linux_gtty __P((struct proc *, struct linux_gtty_args *, int []));
int	linux_access __P((struct proc *, struct linux_access_args *, int []));
int	linux_nice __P((struct proc *, struct linux_nice_args *, int []));
int	linux_ftime __P((struct proc *, struct linux_ftime_args *, int []));
int	linux_kill __P((struct proc *, struct linux_kill_args *, int []));
int	linux_rename __P((struct proc *, struct linux_rename_args *, int []));
int	linux_mkdir __P((struct proc *, struct linux_mkdir_args *, int []));
int	linux_rmdir __P((struct proc *, struct linux_rmdir_args *, int []));
int	linux_pipe __P((struct proc *, struct linux_pipe_args *, int []));
int	linux_times __P((struct proc *, struct linux_times_args *, int []));
int	linux_prof __P((struct proc *, struct linux_prof_args *, int []));
int	linux_brk __P((struct proc *, struct linux_brk_args *, int []));
int	linux_signal __P((struct proc *, struct linux_signal_args *, int []));
int	linux_phys __P((struct proc *, struct linux_phys_args *, int []));
int	linux_lock __P((struct proc *, struct linux_lock_args *, int []));
int	linux_ioctl __P((struct proc *, struct linux_ioctl_args *, int []));
int	linux_fcntl __P((struct proc *, struct linux_fcntl_args *, int []));
int	linux_mpx __P((struct proc *, struct linux_mpx_args *, int []));
int	linux_ulimit __P((struct proc *, struct linux_ulimit_args *, int []));
int	linux_olduname __P((struct proc *, struct linux_olduname_args *, int []));
int	linux_ustat __P((struct proc *, struct linux_ustat_args *, int []));
int	linux_sigaction __P((struct proc *, struct linux_sigaction_args *, int []));
int	linux_siggetmask __P((struct proc *, struct linux_siggetmask_args *, int []));
int	linux_sigsetmask __P((struct proc *, struct linux_sigsetmask_args *, int []));
int	linux_sigsuspend __P((struct proc *, struct linux_sigsuspend_args *, int []));
int	linux_sigpending __P((struct proc *, struct linux_sigpending_args *, int []));
int	linux_select __P((struct proc *, struct linux_select_args *, int []));
int	linux_symlink __P((struct proc *, struct linux_symlink_args *, int []));
int	linux_readlink __P((struct proc *, struct linux_readlink_args *, int []));
int	linux_uselib __P((struct proc *, struct linux_uselib_args *, int []));
int	linux_readdir __P((struct proc *, struct linux_readdir_args *, int []));
int	linux_mmap __P((struct proc *, struct linux_mmap_args *, int []));
int	linux_truncate __P((struct proc *, struct linux_truncate_args *, int []));
int	linux_statfs __P((struct proc *, struct linux_statfs_args *, int []));
int	linux_fstatfs __P((struct proc *, struct linux_fstatfs_args *, int []));
int	linux_ioperm __P((struct proc *, struct linux_ioperm_args *, int []));
int	linux_socketcall __P((struct proc *, struct linux_socketcall_args *, int []));
int	linux_ksyslog __P((struct proc *, struct linux_ksyslog_args *, int []));
int	linux_setitimer __P((struct proc *, struct linux_setitimer_args *, int []));
int	linux_getitimer __P((struct proc *, struct linux_getitimer_args *, int []));
int	linux_newstat __P((struct proc *, struct linux_newstat_args *, int []));
int	linux_newlstat __P((struct proc *, struct linux_newlstat_args *, int []));
int	linux_newfstat __P((struct proc *, struct linux_newfstat_args *, int []));
int	linux_uname __P((struct proc *, struct linux_uname_args *, int []));
int	linux_iopl __P((struct proc *, struct linux_iopl_args *, int []));
int	linux_vhangup __P((struct proc *, struct linux_vhangup_args *, int []));
int	linux_idle __P((struct proc *, struct linux_idle_args *, int []));
int	linux_vm86 __P((struct proc *, struct linux_vm86_args *, int []));
int	linux_wait4 __P((struct proc *, struct linux_wait4_args *, int []));
int	linux_swapoff __P((struct proc *, struct linux_swapoff_args *, int []));
int	linux_sysinfo __P((struct proc *, struct linux_sysinfo_args *, int []));
int	linux_ipc __P((struct proc *, struct linux_ipc_args *, int []));
int	linux_sigreturn __P((struct proc *, struct linux_sigreturn_args *, int []));
int	linux_clone __P((struct proc *, struct linux_clone_args *, int []));
int	linux_newuname __P((struct proc *, struct linux_newuname_args *, int []));
int	linux_modify_ldt __P((struct proc *, struct linux_modify_ldt_args *, int []));
int	linux_adjtimex __P((struct proc *, struct linux_adjtimex_args *, int []));
int	linux_sigprocmask __P((struct proc *, struct linux_sigprocmask_args *, int []));
int	linux_create_module __P((struct proc *, struct linux_create_module_args *, int []));
int	linux_init_module __P((struct proc *, struct linux_init_module_args *, int []));
int	linux_delete_module __P((struct proc *, struct linux_delete_module_args *, int []));
int	linux_get_kernel_syms __P((struct proc *, struct linux_get_kernel_syms_args *, int []));
int	linux_quotactl __P((struct proc *, struct linux_quotactl_args *, int []));
int	linux_getpgid __P((struct proc *, struct linux_getpgid_args *, int []));
int	linux_bdflush __P((struct proc *, struct linux_bdflush_args *, int []));
int	linux_personality __P((struct proc *, struct linux_personality_args *, int []));
int	linux_llseek __P((struct proc *, struct linux_llseek_args *, int []));
int	linux_getdents __P((struct proc *, struct linux_getdents_args *, int []));
int	linux_newselect __P((struct proc *, struct linux_newselect_args *, int []));
int	linux_mremap __P((struct proc *, struct linux_mremap_args *, int []));
int	linux_msync __P((struct proc *, struct linux_msync_args *, int []));

#ifdef COMPAT_43


#endif /* COMPAT_43 */

#undef PAD_

#endif /* !_LINUX_SYSPROTO_H_ */

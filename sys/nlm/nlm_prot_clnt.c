/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <sys/param.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <nlm/nlm_prot.h>
#include <sys/cdefs.h>
#ifndef lint
/*static char sccsid[] = "from: @(#)nlm_prot.x 1.8 87/09/21 Copyr 1987 Sun Micro";*/
/*static char sccsid[] = "from: * @(#)nlm_prot.x	2.1 88/08/01 4.0 RPCSRC";*/
__RCSID("$NetBSD: nlm_prot.x,v 1.6 2000/06/07 14:30:15 bouyer Exp $");
#endif /* not lint */
__FBSDID("$FreeBSD$");

enum clnt_stat 
nlm_sm_notify_0(struct nlm_sm_status *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_SM_NOTIFY,
		(xdrproc_t) xdr_nlm_sm_status, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_test_1(struct nlm_testargs *argp, nlm_testres *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_TEST,
		(xdrproc_t) xdr_nlm_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_testres, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_lock_1(struct nlm_lockargs *argp, nlm_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_LOCK,
		(xdrproc_t) xdr_nlm_lockargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_cancel_1(struct nlm_cancargs *argp, nlm_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_CANCEL,
		(xdrproc_t) xdr_nlm_cancargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_unlock_1(struct nlm_unlockargs *argp, nlm_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_UNLOCK,
		(xdrproc_t) xdr_nlm_unlockargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_granted_1(struct nlm_testargs *argp, nlm_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_GRANTED,
		(xdrproc_t) xdr_nlm_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_test_msg_1(struct nlm_testargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_TEST_MSG,
		(xdrproc_t) xdr_nlm_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_lock_msg_1(struct nlm_lockargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_LOCK_MSG,
		(xdrproc_t) xdr_nlm_lockargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_cancel_msg_1(struct nlm_cancargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_CANCEL_MSG,
		(xdrproc_t) xdr_nlm_cancargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_unlock_msg_1(struct nlm_unlockargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_UNLOCK_MSG,
		(xdrproc_t) xdr_nlm_unlockargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_granted_msg_1(struct nlm_testargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_GRANTED_MSG,
		(xdrproc_t) xdr_nlm_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_test_res_1(nlm_testres *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_TEST_RES,
		(xdrproc_t) xdr_nlm_testres, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_lock_res_1(nlm_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_LOCK_RES,
		(xdrproc_t) xdr_nlm_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_cancel_res_1(nlm_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_CANCEL_RES,
		(xdrproc_t) xdr_nlm_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_unlock_res_1(nlm_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_UNLOCK_RES,
		(xdrproc_t) xdr_nlm_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_granted_res_1(nlm_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_GRANTED_RES,
		(xdrproc_t) xdr_nlm_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_share_3(nlm_shareargs *argp, nlm_shareres *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_SHARE,
		(xdrproc_t) xdr_nlm_shareargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_shareres, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_unshare_3(nlm_shareargs *argp, nlm_shareres *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_UNSHARE,
		(xdrproc_t) xdr_nlm_shareargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_shareres, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_nm_lock_3(nlm_lockargs *argp, nlm_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_NM_LOCK,
		(xdrproc_t) xdr_nlm_lockargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm_free_all_3(nlm_notify *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM_FREE_ALL,
		(xdrproc_t) xdr_nlm_notify, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_test_4(nlm4_testargs *argp, nlm4_testres *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_TEST,
		(xdrproc_t) xdr_nlm4_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_testres, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_lock_4(nlm4_lockargs *argp, nlm4_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_LOCK,
		(xdrproc_t) xdr_nlm4_lockargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_cancel_4(nlm4_cancargs *argp, nlm4_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_CANCEL,
		(xdrproc_t) xdr_nlm4_cancargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_unlock_4(nlm4_unlockargs *argp, nlm4_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_UNLOCK,
		(xdrproc_t) xdr_nlm4_unlockargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_granted_4(nlm4_testargs *argp, nlm4_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_GRANTED,
		(xdrproc_t) xdr_nlm4_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_test_msg_4(nlm4_testargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_TEST_MSG,
		(xdrproc_t) xdr_nlm4_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_lock_msg_4(nlm4_lockargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_LOCK_MSG,
		(xdrproc_t) xdr_nlm4_lockargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_cancel_msg_4(nlm4_cancargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_CANCEL_MSG,
		(xdrproc_t) xdr_nlm4_cancargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_unlock_msg_4(nlm4_unlockargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_UNLOCK_MSG,
		(xdrproc_t) xdr_nlm4_unlockargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_granted_msg_4(nlm4_testargs *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_GRANTED_MSG,
		(xdrproc_t) xdr_nlm4_testargs, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_test_res_4(nlm4_testres *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_TEST_RES,
		(xdrproc_t) xdr_nlm4_testres, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_lock_res_4(nlm4_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_LOCK_RES,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_cancel_res_4(nlm4_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_CANCEL_RES,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_unlock_res_4(nlm4_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_UNLOCK_RES,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_granted_res_4(nlm4_res *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_GRANTED_RES,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_share_4(nlm4_shareargs *argp, nlm4_shareres *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_SHARE,
		(xdrproc_t) xdr_nlm4_shareargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_shareres, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_unshare_4(nlm4_shareargs *argp, nlm4_shareres *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_UNSHARE,
		(xdrproc_t) xdr_nlm4_shareargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_shareres, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_nm_lock_4(nlm4_lockargs *argp, nlm4_res *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_NM_LOCK,
		(xdrproc_t) xdr_nlm4_lockargs, (caddr_t) argp,
		(xdrproc_t) xdr_nlm4_res, (caddr_t) clnt_res,
		timo));
}

enum clnt_stat 
nlm4_free_all_4(nlm4_notify *argp, void *clnt_res, CLIENT *clnt, struct rpc_callextra *ext, struct timeval timo)
{
	return (CLNT_CALL_EXT(clnt, ext, NLM4_FREE_ALL,
		(xdrproc_t) xdr_nlm4_notify, (caddr_t) argp,
		(xdrproc_t) xdr_void, (caddr_t) clnt_res,
		timo));
}

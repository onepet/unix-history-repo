/*-
 * Copyright (c) 1999, 2000, 2001 Robert N. M. Watson
 * All rights reserved.
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
 * $FreeBSD$
 */
/*
 * acl_get_file - syscall wrapper for retrieving ACL by filename
 * acl_get_fd - syscall wrapper for retrieving access ACL by fd
 * acl_get_fd_np - syscall wrapper for retrieving ACL by fd (non-POSIX)
 * acl_get_permset() returns the permission set in the ACL entry
 * acl_get_qualifier() retrieves the qualifier of the tag from the ACL entry
 * acl_get_tag_type() returns the tag type for the ACL entry entry_d
 */

#include <sys/types.h>
#include "namespace.h"
#include <sys/acl.h>
#include "un-namespace.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

acl_t
acl_get_file(const char *path_p, acl_type_t type)
{
	struct acl	*aclp;
	int	error;

	aclp = acl_init(ACL_MAX_ENTRIES);
	if (!aclp) {
		return (NULL);
	}

	error = __acl_get_file(path_p, type, aclp);
	if (error) {
		acl_free(aclp);
		return (NULL);
	}

	return (aclp);
}

acl_t
acl_get_fd(int fd)
{
	struct acl	*aclp;
	int	error;

	aclp = acl_init(ACL_MAX_ENTRIES);
	if (!aclp) {
		return (NULL);
	}

	error = ___acl_get_fd(fd, ACL_TYPE_ACCESS, aclp);
	if (error) {
		acl_free(aclp);
		return (NULL);
	}

	return (aclp);
}

acl_t
acl_get_fd_np(int fd, acl_type_t type)
{
	struct acl	*aclp;
	int	error;

	aclp = acl_init(ACL_MAX_ENTRIES);
	if (!aclp) {
		return (NULL);
	}

	error = ___acl_get_fd(fd, type, aclp);
	if (error) {
		acl_free(aclp);
		return (NULL);
	}

	return (aclp);
}

int
acl_get_permset(acl_entry_t entry_d, acl_permset_t *permset_p)
{

	if (!entry_d || !permset_p) {
		errno = EINVAL;
		return -1;
	}

	*permset_p = &entry_d->ae_perm;

	return 0;
}

void *
acl_get_qualifier(acl_entry_t entry_d)
{
	uid_t *retval;

	if (!entry_d) {
		errno = EINVAL;
		return NULL;
	}

	switch(entry_d->ae_tag) {
	case ACL_USER:
	case ACL_GROUP:
		retval = malloc(sizeof(uid_t));
		if (retval) {
			*retval = entry_d->ae_id;
			return retval;
		}
	}

	errno = EINVAL;
	return NULL;
}

int
acl_get_tag_type(acl_entry_t entry_d, acl_tag_t *tag_type_p)
{

	if (!entry_d || !tag_type_p) {
		errno = EINVAL;
		return -1;
	}

	*tag_type_p = entry_d->ae_tag;

	return 0;
}

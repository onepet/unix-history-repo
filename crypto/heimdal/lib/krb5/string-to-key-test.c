/*
 * Copyright (c) 1999 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of KTH nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY KTH AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL KTH OR ITS CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include "krb5_locl.h"

RCSID("$Id: string-to-key-test.c,v 1.2 1999/10/28 23:10:38 assar Exp $");

enum { MAXSIZE = 24 };

static struct testcase {
    const char *principal_name;
    const char *password;
    krb5_enctype enctype;
    unsigned char res[MAXSIZE];
} tests[] = {
    {"@", "", ETYPE_DES_CBC_MD5,
     {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}},
    {"nisse@FOO.SE", "hej", ETYPE_DES_CBC_MD5,
     {0xfe, 0x67, 0xbf, 0x9e, 0x57, 0x6b, 0xfe, 0x52}},
    {"assar/liten@FOO.SE", "hemligt", ETYPE_DES_CBC_MD5,
     {0x5b, 0x9b, 0xcb, 0xf2, 0x97, 0x43, 0xc8, 0x40}},
    {"@", "", ETYPE_DES3_CBC_SHA1,
     {0xce, 0xa2, 0x2f, 0x9b, 0x52, 0x2c, 0xb0, 0x15, 0x6e, 0x6b, 0x64,
      0x73, 0x62, 0x64, 0x73, 0x4f, 0x6e, 0x73, 0xce, 0xa2, 0x2f, 0x9b,
      0x52, 0x57}},
    {"nisse@FOO.SE", "hej", ETYPE_DES3_CBC_SHA1,
     {0x0e, 0xbc, 0x23, 0x9d, 0x68, 0x46, 0xf2, 0xd5, 0x51, 0x98, 0x5b,
      0x57, 0xc1, 0x57, 0x01, 0x79, 0x04, 0xc4, 0xe9, 0xfe, 0xc1, 0x0e,
      0x13, 0xd0}},
    {"assar/liten@FOO.SE", "hemligt", ETYPE_DES3_CBC_SHA1,
     {0x7f, 0x40, 0x67, 0xb9, 0xbc, 0xc4, 0x40, 0xfb, 0x43, 0x73, 0xd9,
      0xd3, 0xcd, 0x7c, 0xc7, 0x67, 0xe6, 0x79, 0x94, 0xd0, 0xa8, 0x34,
      0xdf, 0x62}},
    {NULL}
};

int
main(int argc, char **argv)
{
    struct testcase *t;
    krb5_context context;
    krb5_error_code ret;
    int val = 0;

    krb5_init_context (&context);

    for (t = tests; t->principal_name; ++t) {
	krb5_keyblock key;
	krb5_principal principal;
	int i;

	ret = krb5_parse_name (context, t->principal_name, &principal);
	if (ret)
	    krb5_err (context, 1, ret, "krb5_parse_name %s",
		      t->principal_name);
	ret = krb5_string_to_key (context, t->enctype, t->password,
				  principal, &key);
	if (ret)
	    krb5_err (context, 1, ret, "krb5_string_to_key");
	krb5_free_principal (context, principal);
	if (memcmp (key.keyvalue.data, t->res, key.keyvalue.length) != 0) {
	    const unsigned char *p = key.keyvalue.data;

	    printf ("string_to_key(%s, %s) failed\n",
		    t->principal_name, t->password);
	    printf ("should be: ");
	    for (i = 0; i < key.keyvalue.length; ++i)
		printf ("%02x", t->res[i]);
	    printf ("\nresult was: ");
	    for (i = 0; i < key.keyvalue.length; ++i)
		printf ("%02x", p[i]);
	    printf ("\n");
	    val = 1;
	}
    }
    return val;
}

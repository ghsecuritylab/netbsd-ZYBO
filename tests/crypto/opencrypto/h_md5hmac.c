/* $NetBSD: h_md5hmac.c,v 1.3 2014/01/17 22:31:25 pgoyette Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/time.h>

#include <crypto/cryptodev.h>

#define	MD5_HMAC_KEYLEN		16	/* Fixed key length supported */

/* Test data from RFC2202 */
const struct {
	int num;
        size_t key_len;
        size_t len;
	unsigned char key[80];
	unsigned char data[80];
        unsigned char mac[16];
} tests[] = {
	/* Test #1 */
	{ 1, 16, 8,
	  { 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
	    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b },
	  "Hi There",
	  { 0x92, 0x94, 0x72, 0x7a, 0x36, 0x38, 0xbb, 0x1c,
	    0x13, 0xf4, 0x8e, 0xf8, 0x15, 0x8b, 0xfc, 0x9d }
	},
	/* Test #2 */
	{ 2, 4, 28,
	  "Jefe",
	  "what do ya want for nothing?",
	  { 0x75, 0x0c, 0x78, 0x3e, 0x6a, 0xb0, 0xb5, 0x03,
	    0xea, 0xa8, 0x6e, 0x31, 0x0a, 0x5d, 0xb7, 0x38 }
	},
	/* Test #3 */
	{ 3, 16, 50,
	  { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
	  { 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
	    0xdd, 0xdd },
	  { 0x56, 0xbe, 0x34, 0x52, 0x1d, 0x14, 0x4c, 0x88,
	    0xdb, 0xb8, 0xc7, 0x33, 0xf0, 0xe8, 0xb3, 0xf6 }
	},
	/* Test #4 */
	{ 4, 25, 50,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
	    0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
	    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
	    0x19 },
	  { 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
	    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
	    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
	    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
	    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
	    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
	    0xcd, 0xcd },
	  { 0x69, 0x7e, 0xaf, 0x0a, 0xca, 0x3a, 0x3a, 0xea,
	    0x3a, 0x75, 0x16, 0x47, 0x46, 0xff, 0xaa, 0x79 }
	},
	/* Test #5 */
	{ 5, 16, 20,
	  { 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
	    0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c },
	  "Test With Truncation",
	  { 0x56, 0x46, 0x1e, 0xf2, 0x34, 0x2e, 0xdc, 0x00,
	    0xf9, 0xba, 0xb9, 0x95, 0x69, 0x0e, 0xfd, 0x4c }
	},
	/* Test #6 */
	{ 6, 80, 54, 
	  { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
	  "Test Using Larger Than Block-Size Key - Hash Key First",
	  { 0x6b, 0x1a, 0xb7, 0xfe, 0x4b, 0xd7, 0xbf, 0x8f,
	    0x0b, 0x62, 0xe6, 0xce, 0x61, 0xb9, 0xd0, 0xcd }
	},
	/* Test #7 */
	{ 7, 80, 73, 
	  { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa },
	  "Test Using Larger Than Block-Size Key and Larger "
		"Than One Block-Size Data",
	{ 0x6f, 0x63, 0x0f, 0xad, 0x67, 0xcd, 0xa0, 0xee,
	  0x1f, 0xb1, 0xf5, 0x62, 0xdb, 0x3a, 0xa5, 0x3e }
	},
};

int
main(void)
{
	size_t i;
	int fd, res;
	struct session_op cs;
	struct crypt_op co;
	unsigned char buf[16];

	fd = open("/dev/crypto", O_RDWR, 0);
	if (fd < 0)
		err(1, "open");
	for (i = 0; i < __arraycount(tests); i++) {
		if (tests[i].key_len != MD5_HMAC_KEYLEN)
			continue;

		memset(&cs, 0, sizeof(cs));
		cs.mac = CRYPTO_MD5_HMAC;
		cs.mackeylen = tests[i].key_len;
		cs.mackey = __UNCONST(&tests[i].key);
		res = ioctl(fd, CIOCGSESSION, &cs);
		if (res < 0)
			err(1, "CIOCGSESSION test %d", tests[i].num);

		memset(&co, 0, sizeof(co));
		memset(buf, 0, sizeof(buf));
		co.ses = cs.ses;
		co.op = COP_ENCRYPT;
		co.len = tests[i].len;
		co.src = __UNCONST(&tests[i].data);
		co.mac = buf;
		res = ioctl(fd, CIOCCRYPT, &co);
		if (res < 0)
			err(1, "CIOCCRYPT test %d", tests[i].num);

		if (memcmp(co.mac, tests[i].mac, sizeof(tests[i].mac)))
			errx(1, "verification failed test %d", tests[i].num);

		res = ioctl(fd, CIOCFSESSION, &cs.ses);
		if (res < 0)
			err(1, "CIOCFSESSION test %d", tests[i].num);
	}
	return 0;
}
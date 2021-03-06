/* $NetBSD$ */

/*-
 * Copyright (c) 2017 Internet Initiative Japan Inc.
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

unsigned char key[] =
	"abcdefgh" \
	"ijklmnop" \
	"qrstuvwx";
unsigned char iv[8] = {0};
char plaintx[16] = "1234567890123456";

const unsigned char ciphertx[16] = {
	0xe0, 0xb9, 0xe7, 0x20, 0x6c, 0xc7, 0xb0, 0x24,
	0xfa, 0xfc, 0x46, 0x1b, 0xad, 0xc1, 0xef, 0x4e,
};

int
main(void)
{
	int fd, res;
	struct session_op cs;
	struct crypt_op co, co2;
	unsigned char buf[16], buf2[16];

	fd = open("/dev/crypto", O_RDWR, 0);
	if (fd < 0)
		err(1, "open");
	memset(&cs, 0, sizeof(cs));
	cs.cipher = CRYPTO_3DES_CBC;
	cs.keylen = 24;
	cs.key = key;
	res = ioctl(fd, CIOCGSESSION, &cs);
	if (res < 0)
		err(1, "CIOCGSESSION");

	memset(&co, 0, sizeof(co));
	memset(&buf, 0, sizeof(buf));
	co.ses = cs.ses;
	co.op = COP_ENCRYPT;
	co.len = sizeof(plaintx);
	co.src = plaintx;
	co.dst = buf;
	co.dst_len = sizeof(buf);
	co.iv = iv;
	res = ioctl(fd, CIOCCRYPT, &co);
	if (res < 0)
		err(1, "CIOCCRYPT encrypto");

	if (memcmp(co.dst, ciphertx, sizeof(ciphertx)))
		errx(1, "encrypto verification failed");

	memset(&co2, 0, sizeof(co2));
	memset(&buf2, 0, sizeof(buf2));
	co2.ses = cs.ses;
	co2.op = COP_DECRYPT;
	co2.len = sizeof(buf);
	co2.src = buf;
	co2.dst = buf2;
	co2.dst_len = sizeof(buf2);
	co2.iv = iv;
	res = ioctl(fd, CIOCCRYPT, &co2);
	if (res < 0)
		err(1, "CIOCCRYPT decrypto");

	if (memcmp(co2.dst, plaintx, sizeof(plaintx)))
		errx(1, "decrypto verification failed");

	return 0;
}

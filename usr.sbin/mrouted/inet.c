/*	$NetBSD: inet.c,v 1.11 2003/05/16 22:59:50 dsl Exp $	*/

/*
 * The mrouted program is covered by the license in the accompanying file
 * named "LICENSE".  Use of the mrouted program represents acceptance of
 * the terms and conditions listed in that file.
 *
 * The mrouted program is COPYRIGHT 1989 by The Board of Trustees of
 * Leland Stanford Junior University.
 */


#include "defs.h"


/* buffers to hold the string representations  */
/* of IP addresses, returned by inet_fmt{s}() */
#define SS_MASK	((1 << 3) - 1)
static char ss[SS_MASK + 1][32];
static int ss_index = 0;	/* index into above */


/*
 * Verify that a given IP address is credible as a host address.
 * (Without a mask, cannot detect addresses of the form {subnet,0} or
 * {subnet,-1}.)
 */
int
inet_valid_host(u_int32_t naddr)
{
    u_int32_t addr;

    addr = ntohl(naddr);

    return (!(IN_MULTICAST(addr) ||
	      IN_BADCLASS (addr) ||
	      (addr & 0xff000000) == 0));
}

/*
 * Verify that a given netmask is plausible;
 * make sure that it is a series of 1's followed by
 * a series of 0's with no discontiguous 1's.
 */
int
inet_valid_mask(u_int32_t mask)
{
    if (~(((mask & -mask) - 1) | mask) != 0) {
	/* Mask is not contiguous */
	return (FALSE);
    }

    return (TRUE);
}

/*
 * Verify that a given subnet number and mask pair are credible.
 *
 * With CIDR, almost any subnet and mask are credible.  mrouted still
 * can't handle aggregated class A's, so we still check that, but
 * otherwise the only requirements are that the subnet address is
 * within the [ABC] range and that the host bits of the subnet
 * are all 0.
 */
int
inet_valid_subnet(u_int32_t nsubnet, u_int32_t nmask)
{
    u_int32_t subnet, mask;

    subnet = ntohl(nsubnet);
    mask   = ntohl(nmask);

    if ((subnet & mask) != subnet) return (FALSE);

    if (subnet == 0)
	return (mask == 0);

    if (IN_CLASSA(subnet)) {
	if (mask < 0xff000000 ||
	    (subnet & 0xff000000) == 0x7f000000 ||
	    (subnet & 0xff000000) == 0x00000000) return (FALSE);
    }
    else if (IN_CLASSD(subnet) || IN_BADCLASS(subnet)) {
	/* Above Class C address space */
	return (FALSE);
    }
    if (subnet & ~mask) {
	/* Host bits are set in the subnet */
	return (FALSE);
    }
    if (!inet_valid_mask(mask)) {
	/* Netmask is not contiguous */
	return (FALSE);
    }

    return (TRUE);
}


/*
 * Convert an IP address in u_long (network) format into a printable string.
 */
char *
inet_fmt(u_int32_t addr)
{
    u_char *a;
    char *s = ss[++ss_index & SS_MASK];

    a = (u_char *)&addr;
    snprintf(s, sizeof ss[0], "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
    return (s);
}


/*
 * Convert an IP subnet number in u_long (network) format into a printable
 * string including the netmask as a number of bits.
 */
char *
inet_fmts(u_int32_t addr, u_int32_t mask)
{
    u_char *a, *m;
    int bits;
    char *s = ss[++ss_index & SS_MASK];

    if ((addr == 0) && (mask == 0)) {
	snprintf(s, sizeof ss[0], "default");
	return (s);
    }
    a = (u_char *)&addr;
    m = (u_char *)&mask;
    bits = 33 - ffs(ntohl(mask));

    if      (m[3] != 0) snprintf(s, sizeof ss[0], "%u.%u.%u.%u/%d", a[0], a[1], a[2], a[3],
						bits);
    else if (m[2] != 0) snprintf(s, sizeof ss[0], "%u.%u.%u/%d",    a[0], a[1], a[2], bits);
    else if (m[1] != 0) snprintf(s, sizeof ss[0], "%u.%u/%d",       a[0], a[1], bits);
    else                snprintf(s, sizeof ss[0], "%u/%d",          a[0], bits);

    return (s);
}

/*
 * Convert the printable string representation of an IP address into the
 * u_long (network) format.  Return 0xffffffff on error.  (To detect the
 * legal address with that value, you must explicitly compare the string
 * with "255.255.255.255".)
 */
u_int32_t
inet_parse(char *s, int *mask_p)
{
    u_int32_t a = 0;
    u_int a0, a1, a2, a3;
    char c;
    int n;

    if (sscanf(s, "%u.%u.%u.%u%n", &a0, &a1, &a2, &a3, &n) != 4)
	return 0xffffffff;
    if (a0 > 255 || a1 > 255 || a2 > 255 || a3 > 255)
	return 0xffffffff;

    if (mask_p == 0) {
	if (s[n] != 0)
	    return 0xffffffff;
    } else {
	if (sscanf(s + n, "/%u%c", &n, &c) != 1 || n > 32)
	    return 0xffffffff;
	*mask_p = n;
    }

    ((u_char *)&a)[0] = a0;
    ((u_char *)&a)[1] = a1;
    ((u_char *)&a)[2] = a2;
    ((u_char *)&a)[3] = a3;

    return (a);
}


/*
 * inet_cksum extracted from:
 *			P I N G . C
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 * Modified at Uc Berkeley
 *
 * (ping.c) Status -
 *	Public Domain.  Distribution Unlimited.
 *
 *			I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 */
int
inet_cksum(u_int16_t *addr, u_int len)
{
	int nleft = (int)len;
	u_int16_t *w = addr;
	int32_t sum = 0;
	union {
		u_int16_t w;
		u_int8_t b[2];
	} answer;

	/*
	 *  Our algorithm is simple, using a 32 bit accumulator (sum),
	 *  we add sequential 16 bit words to it, and at the end, fold
	 *  back all the carry bits from the top 16 bits into the lower
	 *  16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		answer.w = 0;
		answer.b[0] = *(u_char *)w ;
		sum += answer.w;
	}

	/*
	 * add back carry outs from top 16 bits to low 16 bits
	 */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer.w = ~sum;			/* truncate to 16 bits */
	return (answer.w);
}

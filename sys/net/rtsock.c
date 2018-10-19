/*	$NetBSD: rtsock.c,v 1.242 2018/08/31 15:15:23 maxv Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)rtsock.c	8.7 (Berkeley) 10/12/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtsock.c,v 1.242 2018/08/31 15:15:23 maxv Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_mpls.h"
#include "opt_compat_netbsd.h"
#include "opt_sctp.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/intr.h>
#include <sys/condvar.h>

#include <net/if.h>
#include <net/if_llatbl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/raw_cb.h>

#include <netinet/in_var.h>
#include <netinet/if_inarp.h>

#include <netmpls/mpls.h>

#ifdef SCTP
extern void sctp_add_ip_address(struct ifaddr *);
extern void sctp_delete_ip_address(struct ifaddr *);
#endif

#if defined(COMPAT_14) || defined(COMPAT_50) || defined(COMPAT_70)
#include <compat/net/if.h>
#include <compat/net/route.h>
#endif
#ifdef COMPAT_RTSOCK
#define	RTM_XVERSION	RTM_OVERSION
#define	RTM_XNEWADDR	RTM_ONEWADDR
#define	RTM_XDELADDR	RTM_ODELADDR
#define	RTM_XCHGADDR	RTM_OCHGADDR
#define	RT_XADVANCE(a,b) RT_OADVANCE(a,b)
#define	RT_XROUNDUP(n)	RT_OROUNDUP(n)
#define	PF_XROUTE	PF_OROUTE
#define	rt_xmsghdr	rt_msghdr50
#define	if_xmsghdr	if_msghdr	/* if_msghdr50 is for RTM_OIFINFO */
#define	ifa_xmsghdr	ifa_msghdr50
#define	if_xannouncemsghdr	if_announcemsghdr50
#define	COMPATNAME(x)	compat_50_ ## x
#define	DOMAINNAME	"oroute"
CTASSERT(sizeof(struct ifa_xmsghdr) == 20);
DOMAIN_DEFINE(compat_50_routedomain); /* forward declare and add to link set */
#undef COMPAT_70
#else /* COMPAT_RTSOCK */
#define	RTM_XVERSION	RTM_VERSION
#define	RTM_XNEWADDR	RTM_NEWADDR
#define	RTM_XDELADDR	RTM_DELADDR
#define	RTM_XCHGADDR	RTM_CHGADDR
#define	RT_XADVANCE(a,b) RT_ADVANCE(a,b)
#define	RT_XROUNDUP(n)	RT_ROUNDUP(n)
#define	PF_XROUTE	PF_ROUTE
#define	rt_xmsghdr	rt_msghdr
#define	if_xmsghdr	if_msghdr
#define	ifa_xmsghdr	ifa_msghdr
#define	if_xannouncemsghdr	if_announcemsghdr
#define	COMPATNAME(x)	x
#define	DOMAINNAME	"route"
CTASSERT(sizeof(struct ifa_xmsghdr) == 32);
#ifdef COMPAT_50
#define	COMPATCALL(name, args)	compat_50_ ## name args
#endif
DOMAIN_DEFINE(routedomain); /* forward declare and add to link set */
#undef COMPAT_50
#undef COMPAT_14
#endif /* COMPAT_RTSOCK */

#ifndef COMPATCALL
#define	COMPATCALL(name, args)	do { } while (/*CONSTCOND*/ 0)
#endif

#ifdef RTSOCK_DEBUG
#define RT_IN_PRINT(info, b, a) (in_print((b), sizeof(b), \
    &((const struct sockaddr_in *)(info)->rti_info[(a)])->sin_addr), (b))
#endif /* RTSOCK_DEBUG */

struct route_info COMPATNAME(route_info) = {
	.ri_dst = { .sa_len = 2, .sa_family = PF_XROUTE, },
	.ri_src = { .sa_len = 2, .sa_family = PF_XROUTE, },
	.ri_maxqlen = IFQ_MAXLEN,
};

static void COMPATNAME(route_init)(void);
static int COMPATNAME(route_output)(struct mbuf *, struct socket *);

static int rt_xaddrs(u_char, const char *, const char *, struct rt_addrinfo *);
static struct mbuf *rt_makeifannouncemsg(struct ifnet *, int, int,
    struct rt_addrinfo *);
static int rt_msg2(int, struct rt_addrinfo *, void *, struct rt_walkarg *, int *);
static void _rt_setmetrics(int, const struct rt_xmsghdr *, struct rtentry *);
static void rtm_setmetrics(const struct rtentry *, struct rt_xmsghdr *);
static void sysctl_net_route_setup(struct sysctllog **);
static int sysctl_dumpentry(struct rtentry *, void *);
static int sysctl_iflist(int, struct rt_walkarg *, int);
static int sysctl_rtable(SYSCTLFN_PROTO);
static void rt_adjustcount(int, int);

static const struct protosw COMPATNAME(route_protosw)[];

struct routecb {
	struct rawcb	rocb_rcb;
	unsigned int	rocb_msgfilter;
#define	RTMSGFILTER(m)	(1U << (m))
};
#define sotoroutecb(so)	((struct routecb *)(so)->so_pcb)

static struct rawcbhead rt_rawcb;
#ifdef NET_MPSAFE
static kmutex_t *rt_so_mtx;

static bool rt_updating = false;
static kcondvar_t rt_update_cv;
#endif

static void
rt_adjustcount(int af, int cnt)
{
	struct route_cb * const cb = &COMPATNAME(route_info).ri_cb;

	cb->any_count += cnt;

	switch (af) {
	case AF_INET:
		cb->ip_count += cnt;
		return;
#ifdef INET6
	case AF_INET6:
		cb->ip6_count += cnt;
		return;
#endif
	case AF_MPLS:
		cb->mpls_count += cnt;
		return;
	}
}

static int
COMPATNAME(route_filter)(struct mbuf *m, struct sockproto *proto,
    struct rawcb *rp)
{
	struct routecb *rop = (struct routecb *)rp;
	struct rt_xmsghdr *rtm;

	KASSERT(m != NULL);
	KASSERT(proto != NULL);
	KASSERT(rp != NULL);

	/* Wrong family for this socket. */
	if (proto->sp_family != PF_ROUTE)
		return ENOPROTOOPT;

	/* If no filter set, just return. */
	if (rop->rocb_msgfilter == 0)
		return 0;

	/* Ensure we can access rtm_type */
	if (m->m_len <
	    offsetof(struct rt_xmsghdr, rtm_type) + sizeof(rtm->rtm_type))
		return EINVAL;

	rtm = mtod(m, struct rt_xmsghdr *);
	/* If the rtm type is filtered out, return a positive. */
	if (!(rop->rocb_msgfilter & RTMSGFILTER(rtm->rtm_type)))
		return EEXIST;

	/* Passed the filter. */
	return 0;
}

static void
rt_pr_init(void)
{

	LIST_INIT(&rt_rawcb);
}

static int
COMPATNAME(route_attach)(struct socket *so, int proto)
{
	struct rawcb *rp;
	struct routecb *rop;
	int s, error;

	KASSERT(sotorawcb(so) == NULL);
	rop = kmem_zalloc(sizeof(*rop), KM_SLEEP);
	rp = &rop->rocb_rcb;
	rp->rcb_len = sizeof(*rop);
	so->so_pcb = rp;

	s = splsoftnet();

#ifdef NET_MPSAFE
	KASSERT(so->so_lock == NULL);
	mutex_obj_hold(rt_so_mtx);
	so->so_lock = rt_so_mtx;
	solock(so);
#endif

	if ((error = raw_attach(so, proto, &rt_rawcb)) == 0) {
		rt_adjustcount(rp->rcb_proto.sp_protocol, 1);
		rp->rcb_laddr = &COMPATNAME(route_info).ri_src;
		rp->rcb_faddr = &COMPATNAME(route_info).ri_dst;
		rp->rcb_filter = COMPATNAME(route_filter);
	}
	splx(s);

	if (error) {
		kmem_free(rop, sizeof(*rop));
		so->so_pcb = NULL;
		return error;
	}

	soisconnected(so);
	so->so_options |= SO_USELOOPBACK;
	KASSERT(solocked(so));

	return error;
}

static void
COMPATNAME(route_detach)(struct socket *so)
{
	struct rawcb *rp = sotorawcb(so);
	int s;

	KASSERT(rp != NULL);
	KASSERT(solocked(so));

	s = splsoftnet();
	rt_adjustcount(rp->rcb_proto.sp_protocol, -1);
	raw_detach(so);
	splx(s);
}

static int
COMPATNAME(route_accept)(struct socket *so, struct sockaddr *nam)
{
	KASSERT(solocked(so));

	panic("route_accept");

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_bind)(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_listen)(struct socket *so, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_connect)(struct socket *so, struct sockaddr *nam, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_connect2)(struct socket *so, struct socket *so2)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_disconnect)(struct socket *so)
{
	struct rawcb *rp = sotorawcb(so);
	int s;

	KASSERT(solocked(so));
	KASSERT(rp != NULL);

	s = splsoftnet();
	soisdisconnected(so);
	raw_disconnect(rp);
	splx(s);

	return 0;
}

static int
COMPATNAME(route_shutdown)(struct socket *so)
{
	int s;

	KASSERT(solocked(so));

	/*
	 * Mark the connection as being incapable of further input.
	 */
	s = splsoftnet();
	socantsendmore(so);
	splx(s);
	return 0;
}

static int
COMPATNAME(route_abort)(struct socket *so)
{
	KASSERT(solocked(so));

	panic("route_abort");

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_ioctl)(struct socket *so, u_long cmd, void *nam,
    struct ifnet * ifp)
{
	return EOPNOTSUPP;
}

static int
COMPATNAME(route_stat)(struct socket *so, struct stat *ub)
{
	KASSERT(solocked(so));

	return 0;
}

static int
COMPATNAME(route_peeraddr)(struct socket *so, struct sockaddr *nam)
{
	struct rawcb *rp = sotorawcb(so);

	KASSERT(solocked(so));
	KASSERT(rp != NULL);
	KASSERT(nam != NULL);

	if (rp->rcb_faddr == NULL)
		return ENOTCONN;

	raw_setpeeraddr(rp, nam);
	return 0;
}

static int
COMPATNAME(route_sockaddr)(struct socket *so, struct sockaddr *nam)
{
	struct rawcb *rp = sotorawcb(so);

	KASSERT(solocked(so));
	KASSERT(rp != NULL);
	KASSERT(nam != NULL);

	if (rp->rcb_faddr == NULL)
		return ENOTCONN;

	raw_setsockaddr(rp, nam);
	return 0;
}

static int
COMPATNAME(route_rcvd)(struct socket *so, int flags, struct lwp *l)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_recvoob)(struct socket *so, struct mbuf *m, int flags)
{
	KASSERT(solocked(so));

	return EOPNOTSUPP;
}

static int
COMPATNAME(route_send)(struct socket *so, struct mbuf *m,
    struct sockaddr *nam, struct mbuf *control, struct lwp *l)
{
	int error = 0;
	int s;

	KASSERT(solocked(so));
	KASSERT(so->so_proto == &COMPATNAME(route_protosw)[0]);

	s = splsoftnet();
	error = raw_send(so, m, nam, control, l, &COMPATNAME(route_output));
	splx(s);

	return error;
}

static int
COMPATNAME(route_sendoob)(struct socket *so, struct mbuf *m,
    struct mbuf *control)
{
	KASSERT(solocked(so));

	m_freem(m);
	m_freem(control);

	return EOPNOTSUPP;
}
static int
COMPATNAME(route_purgeif)(struct socket *so, struct ifnet *ifp)
{

	panic("route_purgeif");

	return EOPNOTSUPP;
}

#if defined(INET) || defined(INET6)
static int
route_get_sdl_index(struct rt_addrinfo *info, int *sdl_index)
{
	struct rtentry *nrt;
	int error;

	error = rtrequest1(RTM_GET, info, &nrt);
	if (error != 0)
		return error;
	/*
	 * nrt->rt_ifp->if_index may not be correct
	 * due to changing to ifplo0.
	 */
	*sdl_index = satosdl(nrt->rt_gateway)->sdl_index;
	rt_unref(nrt);

	return 0;
}
#endif

static void
route_get_sdl(const struct ifnet *ifp, const struct sockaddr *dst,
    struct sockaddr_dl *sdl, int *flags)
{
	struct llentry *la;

	KASSERT(ifp != NULL);

	IF_AFDATA_RLOCK(ifp);
	switch (dst->sa_family) {
	case AF_INET:
		la = lla_lookup(LLTABLE(ifp), 0, dst);
		break;
	case AF_INET6:
		la = lla_lookup(LLTABLE6(ifp), 0, dst);
		break;
	default:
		la = NULL;
		KASSERTMSG(0, "Invalid AF=%d\n", dst->sa_family);
		break;
	}
	IF_AFDATA_RUNLOCK(ifp);

	void *a = (LLE_IS_VALID(la) && (la->la_flags & LLE_VALID) == LLE_VALID)
	    ? &la->ll_addr : NULL;

	a = sockaddr_dl_init(sdl, sizeof(*sdl), ifp->if_index, ifp->if_type,
	    NULL, 0, a, ifp->if_addrlen);
	KASSERT(a != NULL);

	if (la != NULL) {
		*flags = la->la_flags;
		LLE_RUNLOCK(la);
	}
}

static int
route_output_report(struct rtentry *rt, struct rt_addrinfo *info,
    struct rt_xmsghdr *rtm, struct rt_xmsghdr **new_rtm)
{
	int len;

	if (rtm->rtm_addrs & (RTA_IFP | RTA_IFA)) {
		const struct ifaddr *rtifa;
		const struct ifnet *ifp = rt->rt_ifp;

		info->rti_info[RTAX_IFP] = ifp->if_dl->ifa_addr;
		/* rtifa used to be simply rt->rt_ifa.
		 * If rt->rt_ifa != NULL, then
		 * rt_get_ifa() != NULL.  So this
		 * ought to still be safe. --dyoung
		 */
		rtifa = rt_get_ifa(rt);
		info->rti_info[RTAX_IFA] = rtifa->ifa_addr;
#ifdef RTSOCK_DEBUG
		if (info->rti_info[RTAX_IFA]->sa_family == AF_INET) {
			char ibuf[INET_ADDRSTRLEN];
			char abuf[INET_ADDRSTRLEN];
			printf("%s: copying out RTAX_IFA %s "
			    "for info->rti_info[RTAX_DST] %s "
			    "ifa_getifa %p ifa_seqno %p\n",
			    __func__,
			    RT_IN_PRINT(info, ibuf, RTAX_IFA),
			    RT_IN_PRINT(info, abuf, RTAX_DST),
			    (void *)rtifa->ifa_getifa,
			    rtifa->ifa_seqno);
		}
#endif /* RTSOCK_DEBUG */
		if (ifp->if_flags & IFF_POINTOPOINT)
			info->rti_info[RTAX_BRD] = rtifa->ifa_dstaddr;
		else
			info->rti_info[RTAX_BRD] = NULL;
		rtm->rtm_index = ifp->if_index;
	}
	(void)rt_msg2(rtm->rtm_type, info, NULL, NULL, &len);
	if (len > rtm->rtm_msglen) {
		struct rt_xmsghdr *old_rtm = rtm;
		R_Malloc(*new_rtm, struct rt_xmsghdr *, len);
		if (*new_rtm == NULL)
			return ENOBUFS;
		(void)memcpy(*new_rtm, old_rtm, old_rtm->rtm_msglen);
		rtm = *new_rtm;
	}
	(void)rt_msg2(rtm->rtm_type, info, rtm, NULL, 0);
	rtm->rtm_flags = rt->rt_flags;
	rtm_setmetrics(rt, rtm);
	rtm->rtm_addrs = info->rti_addrs;

	return 0;
}

/*ARGSUSED*/
int
COMPATNAME(route_output)(struct mbuf *m, struct socket *so)
{
	struct sockproto proto = { .sp_family = PF_XROUTE, };
	struct rt_xmsghdr *rtm = NULL;
	struct rt_xmsghdr *old_rtm = NULL, *new_rtm = NULL;
	struct rtentry *rt = NULL;
	struct rtentry *saved_nrt = NULL;
	struct rt_addrinfo info;
	int len, error = 0;
	sa_family_t family;
	struct sockaddr_dl sdl;
	int bound = curlwp_bind();
	bool do_rt_free = false;
	struct sockaddr_storage netmask;

#define senderr(e) do { error = e; goto flush;} while (/*CONSTCOND*/ 0)
	if (m == NULL || ((m->m_len < sizeof(int32_t)) &&
	   (m = m_pullup(m, sizeof(int32_t))) == NULL)) {
		error = ENOBUFS;
		goto out;
	}
	if ((m->m_flags & M_PKTHDR) == 0)
		panic("%s", __func__);
	len = m->m_pkthdr.len;
	if (len < sizeof(*rtm) ||
	    len != mtod(m, struct rt_xmsghdr *)->rtm_msglen) {
		info.rti_info[RTAX_DST] = NULL;
		senderr(EINVAL);
	}
	R_Malloc(rtm, struct rt_xmsghdr *, len);
	if (rtm == NULL) {
		info.rti_info[RTAX_DST] = NULL;
		senderr(ENOBUFS);
	}
	m_copydata(m, 0, len, rtm);
	if (rtm->rtm_version != RTM_XVERSION) {
		info.rti_info[RTAX_DST] = NULL;
		senderr(EPROTONOSUPPORT);
	}
	rtm->rtm_pid = curproc->p_pid;
	memset(&info, 0, sizeof(info));
	info.rti_addrs = rtm->rtm_addrs;
	if (rt_xaddrs(rtm->rtm_type, (const char *)(rtm + 1), len + (char *)rtm,
	    &info)) {
		senderr(EINVAL);
	}
	info.rti_flags = rtm->rtm_flags;
#ifdef RTSOCK_DEBUG
	if (info.rti_info[RTAX_DST]->sa_family == AF_INET) {
		char abuf[INET_ADDRSTRLEN];
		printf("%s: extracted info.rti_info[RTAX_DST] %s\n", __func__,
		    RT_IN_PRINT(&info, abuf, RTAX_DST));
	}
#endif /* RTSOCK_DEBUG */
	if (info.rti_info[RTAX_DST] == NULL ||
	    (info.rti_info[RTAX_DST]->sa_family >= AF_MAX)) {
		senderr(EINVAL);
	}
	if (info.rti_info[RTAX_GATEWAY] != NULL &&
	    (info.rti_info[RTAX_GATEWAY]->sa_family >= AF_MAX)) {
		senderr(EINVAL);
	}

	/*
	 * Verify that the caller has the appropriate privilege; RTM_GET
	 * is the only operation the non-superuser is allowed.
	 */
	if (kauth_authorize_network(curlwp->l_cred, KAUTH_NETWORK_ROUTE,
	    0, rtm, NULL, NULL) != 0)
		senderr(EACCES);

	/*
	 * route(8) passes a sockaddr truncated with prefixlen.
	 * The kernel doesn't expect such sockaddr and need to 
	 * use a buffer that is big enough for the sockaddr expected
	 * (padded with 0's). We keep the original length of the sockaddr.
	 */
	if (info.rti_info[RTAX_NETMASK]) {
		/*
		 * Use the family of RTAX_DST, because RTAX_NETMASK
		 * can have a zero family if it comes from the radix
		 * tree via rt_mask().
		 */
		socklen_t sa_len = sockaddr_getsize_by_family(
		    info.rti_info[RTAX_DST]->sa_family);
		socklen_t masklen = sockaddr_getlen(
		    info.rti_info[RTAX_NETMASK]);
		if (sa_len != 0 && sa_len > masklen) {
			KASSERT(sa_len <= sizeof(netmask));
			memcpy(&netmask, info.rti_info[RTAX_NETMASK], masklen);
			memset((char *)&netmask + masklen, 0, sa_len - masklen);
			info.rti_info[RTAX_NETMASK] = sstocsa(&netmask);
		}
	}

	switch (rtm->rtm_type) {

	case RTM_ADD:
		if (info.rti_info[RTAX_GATEWAY] == NULL) {
			senderr(EINVAL);
		}
#if defined(INET) || defined(INET6)
		/* support for new ARP/NDP code with keeping backcompat */
		if (info.rti_info[RTAX_GATEWAY]->sa_family == AF_LINK) {
			const struct sockaddr_dl *sdlp =
			    satocsdl(info.rti_info[RTAX_GATEWAY]);

			/* Allow routing requests by interface index */
			if (sdlp->sdl_nlen == 0 && sdlp->sdl_alen == 0
			    && sdlp->sdl_slen == 0)
				goto fallback;
			/*
			 * Old arp binaries don't set the sdl_index
			 * so we have to complement it.
			 */
			int sdl_index = sdlp->sdl_index;
			if (sdl_index == 0) {
				error = route_get_sdl_index(&info, &sdl_index);
				if (error != 0)
					goto fallback;
			} else if (
			    info.rti_info[RTAX_DST]->sa_family == AF_INET) {
				/*
				 * XXX workaround for SIN_PROXY case; proxy arp
				 * entry should be in an interface that has
				 * a network route including the destination,
				 * not a local (link) route that may not be a
				 * desired place, for example a tap.
				 */
				const struct sockaddr_inarp *sina =
				    (const struct sockaddr_inarp *)
				    info.rti_info[RTAX_DST];
				if (sina->sin_other & SIN_PROXY) {
					error = route_get_sdl_index(&info,
					    &sdl_index);
					if (error != 0)
						goto fallback;
				}
			}
			error = lla_rt_output(rtm->rtm_type, rtm->rtm_flags,
			    rtm->rtm_rmx.rmx_expire, &info, sdl_index);
			break;
		}
	fallback:
#endif /* defined(INET) || defined(INET6) */
		error = rtrequest1(rtm->rtm_type, &info, &saved_nrt);
		if (error == 0) {
			_rt_setmetrics(rtm->rtm_inits, rtm, saved_nrt);
			rt_unref(saved_nrt);
		}
		break;

	case RTM_DELETE:
#if defined(INET) || defined(INET6)
		/* support for new ARP/NDP code */
		if (info.rti_info[RTAX_GATEWAY] &&
		    (info.rti_info[RTAX_GATEWAY]->sa_family == AF_LINK) &&
		    (rtm->rtm_flags & RTF_LLDATA) != 0) {
			const struct sockaddr_dl *sdlp =
			    satocsdl(info.rti_info[RTAX_GATEWAY]);
			error = lla_rt_output(rtm->rtm_type, rtm->rtm_flags,
			    rtm->rtm_rmx.rmx_expire, &info, sdlp->sdl_index);
			rtm->rtm_flags &= ~RTF_UP;
			break;
		}
#endif
		error = rtrequest1(rtm->rtm_type, &info, &saved_nrt);
		if (error != 0)
			break;

		rt = saved_nrt;
		do_rt_free = true;
		info.rti_info[RTAX_DST] = rt_getkey(rt);
		info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
		info.rti_info[RTAX_NETMASK] = rt_mask(rt);
		info.rti_info[RTAX_TAG] = rt_gettag(rt);
		error = route_output_report(rt, &info, rtm, &new_rtm);
		if (error)
			senderr(error);
		if (new_rtm != NULL) {
			old_rtm = rtm;
			rtm = new_rtm;
		}
		break;

	case RTM_GET:
	case RTM_CHANGE:
	case RTM_LOCK:
                /* XXX This will mask info.rti_info[RTAX_DST] with
		 * info.rti_info[RTAX_NETMASK] before
                 * searching.  It did not used to do that.  --dyoung
		 */
		rt = NULL;
		error = rtrequest1(RTM_GET, &info, &rt);
		if (error != 0)
			senderr(error);
		if (rtm->rtm_type != RTM_GET) {/* XXX: too grotty */
			if (memcmp(info.rti_info[RTAX_DST], rt_getkey(rt),
			    info.rti_info[RTAX_DST]->sa_len) != 0)
				senderr(ESRCH);
			if (info.rti_info[RTAX_NETMASK] == NULL &&
			    rt_mask(rt) != NULL)
				senderr(ETOOMANYREFS);
		}

		/*
		 * XXX if arp/ndp requests an L2 entry, we have to obtain
		 * it from lltable while for the route command we have to
		 * return a route as it is. How to distinguish them?
		 * For newer arp/ndp, RTF_LLDATA flag set by arp/ndp
		 * indicates an L2 entry is requested. For old arp/ndp
		 * binaries, we check RTF_UP flag is NOT set; it works
		 * by the fact that arp/ndp don't set it while the route
		 * command sets it.
		 */
		if (((rtm->rtm_flags & RTF_LLDATA) != 0 ||
		     (rtm->rtm_flags & RTF_UP) == 0) &&
		    rtm->rtm_type == RTM_GET &&
		    sockaddr_cmp(rt_getkey(rt), info.rti_info[RTAX_DST]) != 0) {
			int ll_flags = 0;
			route_get_sdl(rt->rt_ifp, info.rti_info[RTAX_DST], &sdl,
			    &ll_flags);
			info.rti_info[RTAX_GATEWAY] = sstocsa(&sdl);
			error = route_output_report(rt, &info, rtm, &new_rtm);
			if (error)
				senderr(error);
			if (new_rtm != NULL) {
				old_rtm = rtm;
				rtm = new_rtm;
			}
			rtm->rtm_flags |= RTF_LLDATA;
			rtm->rtm_flags &= ~RTF_CONNECTED;
			rtm->rtm_flags |= (ll_flags & LLE_STATIC) ? RTF_STATIC : 0;
			break;
		}

		switch (rtm->rtm_type) {
		case RTM_GET:
			info.rti_info[RTAX_DST] = rt_getkey(rt);
			info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
			info.rti_info[RTAX_NETMASK] = rt_mask(rt);
			info.rti_info[RTAX_TAG] = rt_gettag(rt);
			error = route_output_report(rt, &info, rtm, &new_rtm);
			if (error)
				senderr(error);
			if (new_rtm != NULL) {
				old_rtm = rtm;
				rtm = new_rtm;
			}
			break;

		case RTM_CHANGE:
#ifdef NET_MPSAFE
			/*
			 * Release rt_so_mtx to avoid a deadlock with route_intr
			 * and also serialize updating routes to avoid another.
			 */
			if (rt_updating) {
				/* Release to allow the updater to proceed */
				rt_unref(rt);
				rt = NULL;
			}
			while (rt_updating) {
				error = cv_wait_sig(&rt_update_cv, rt_so_mtx);
				if (error != 0)
					goto flush;
			}
			if (rt == NULL) {
				error = rtrequest1(RTM_GET, &info, &rt);
				if (error != 0)
					goto flush;
			}
			rt_updating = true;
			mutex_exit(rt_so_mtx);

			error = rt_update_prepare(rt);
			if (error == 0) {
				error = rt_update(rt, &info, rtm);
				rt_update_finish(rt);
			}

			mutex_enter(rt_so_mtx);
			rt_updating = false;
			cv_broadcast(&rt_update_cv);
#else
			error = rt_update(rt, &info, rtm);
#endif
			if (error != 0)
				goto flush;
			/*FALLTHROUGH*/
		case RTM_LOCK:
			rt->rt_rmx.rmx_locks &= ~(rtm->rtm_inits);
			rt->rt_rmx.rmx_locks |=
			    (rtm->rtm_inits & rtm->rtm_rmx.rmx_locks);
			break;
		}
		break;

	default:
		senderr(EOPNOTSUPP);
	}

flush:
	if (rtm) {
		if (error)
			rtm->rtm_errno = error;
		else
			rtm->rtm_flags |= RTF_DONE;
	}
	family = info.rti_info[RTAX_DST] ? info.rti_info[RTAX_DST]->sa_family :
	    0;
	/* We cannot free old_rtm until we have stopped using the
	 * pointers in info, some of which may point to sockaddrs
	 * in old_rtm.
	 */
	if (old_rtm != NULL)
		Free(old_rtm);
	if (rt) {
		if (do_rt_free) {
#ifdef NET_MPSAFE
			/*
			 * Release rt_so_mtx to avoid a deadlock with
			 * route_intr.
			 */
			mutex_exit(rt_so_mtx);
			rt_free(rt);
			mutex_enter(rt_so_mtx);
#else
			rt_free(rt);
#endif
		} else
			rt_unref(rt);
	}
    {
	struct rawcb *rp = NULL;
	/*
	 * Check to see if we don't want our own messages.
	 */
	if ((so->so_options & SO_USELOOPBACK) == 0) {
		if (COMPATNAME(route_info).ri_cb.any_count <= 1) {
			if (rtm)
				Free(rtm);
			m_freem(m);
			goto out;
		}
		/* There is another listener, so construct message */
		rp = sotorawcb(so);
	}
	if (rtm) {
		m_copyback(m, 0, rtm->rtm_msglen, rtm);
		if (m->m_pkthdr.len < rtm->rtm_msglen) {
			m_freem(m);
			m = NULL;
		} else if (m->m_pkthdr.len > rtm->rtm_msglen)
			m_adj(m, rtm->rtm_msglen - m->m_pkthdr.len);
		Free(rtm);
	}
	if (rp)
		rp->rcb_proto.sp_family = 0; /* Avoid us */
	if (family)
		proto.sp_protocol = family;
	if (m)
		raw_input(m, &proto, &COMPATNAME(route_info).ri_src,
		    &COMPATNAME(route_info).ri_dst, &rt_rawcb);
	if (rp)
		rp->rcb_proto.sp_family = PF_XROUTE;
    }
out:
	curlwp_bindx(bound);
	return error;
}

static int
route_ctloutput(int op, struct socket *so, struct sockopt *sopt)
{
	struct routecb *rop = sotoroutecb(so);
	int error = 0;
	unsigned char *rtm_type;
	size_t len;
	unsigned int msgfilter;

	KASSERT(solocked(so));

	if (sopt->sopt_level != AF_ROUTE) {
		error = ENOPROTOOPT;
	} else switch (op) {
	case PRCO_SETOPT:
		switch (sopt->sopt_name) {
		case RO_MSGFILTER:
			msgfilter = 0;
			for (rtm_type = sopt->sopt_data, len = sopt->sopt_size;
			     len != 0;
			     rtm_type++, len -= sizeof(*rtm_type))
			{
				/* Guard against overflowing our storage. */
				if (*rtm_type >= sizeof(msgfilter) * CHAR_BIT) {
					error = EOVERFLOW;
					break;
				}
				msgfilter |= RTMSGFILTER(*rtm_type);
			}
			if (error == 0)
				rop->rocb_msgfilter = msgfilter;
			break;
		default:
			error = ENOPROTOOPT;
			break;
		}
		break;
	case PRCO_GETOPT:
		switch (sopt->sopt_name) {
		case RO_MSGFILTER:
			error = ENOTSUP;
			break;
		default:
			error = ENOPROTOOPT;
			break;
		}
	}
	return error;
}

static void
_rt_setmetrics(int which, const struct rt_xmsghdr *in, struct rtentry *out)
{
#define metric(f, e) if (which & (f)) out->rt_rmx.e = in->rtm_rmx.e;
	metric(RTV_RPIPE, rmx_recvpipe);
	metric(RTV_SPIPE, rmx_sendpipe);
	metric(RTV_SSTHRESH, rmx_ssthresh);
	metric(RTV_RTT, rmx_rtt);
	metric(RTV_RTTVAR, rmx_rttvar);
	metric(RTV_HOPCOUNT, rmx_hopcount);
	metric(RTV_MTU, rmx_mtu);
#undef metric
	if (which & RTV_EXPIRE) {
		out->rt_rmx.rmx_expire = in->rtm_rmx.rmx_expire ?
		    time_wall_to_mono(in->rtm_rmx.rmx_expire) : 0;
	}
}

#ifndef COMPAT_RTSOCK
/*
 * XXX avoid using void * once msghdr compat disappears.
 */
void
rt_setmetrics(void *in, struct rtentry *out)
{
	const struct rt_xmsghdr *rtm = in;

	_rt_setmetrics(rtm->rtm_inits, rtm, out);
}
#endif

static void
rtm_setmetrics(const struct rtentry *in, struct rt_xmsghdr *out)
{
#define metric(e) out->rtm_rmx.e = in->rt_rmx.e;
	metric(rmx_recvpipe);
	metric(rmx_sendpipe);
	metric(rmx_ssthresh);
	metric(rmx_rtt);
	metric(rmx_rttvar);
	metric(rmx_hopcount);
	metric(rmx_mtu);
	metric(rmx_locks);
#undef metric
	out->rtm_rmx.rmx_expire = in->rt_rmx.rmx_expire ?
	    time_mono_to_wall(in->rt_rmx.rmx_expire) : 0;
}

static int
rt_xaddrs(u_char rtmtype, const char *cp, const char *cplim,
    struct rt_addrinfo *rtinfo)
{
	const struct sockaddr *sa = NULL;	/* Quell compiler warning */
	int i;

	for (i = 0; i < RTAX_MAX && cp < cplim; i++) {
		if ((rtinfo->rti_addrs & (1 << i)) == 0)
			continue;
		rtinfo->rti_info[i] = sa = (const struct sockaddr *)cp;
		RT_XADVANCE(cp, sa);
	}

	/*
	 * Check for extra addresses specified, except RTM_GET asking
	 * for interface info.
	 */
	if (rtmtype == RTM_GET) {
		if (((rtinfo->rti_addrs &
		    (~((1 << RTAX_IFP) | (1 << RTAX_IFA)))) & (~0U << i)) != 0)
			return 1;
	} else if ((rtinfo->rti_addrs & (~0U << i)) != 0)
		return 1;
	/* Check for bad data length.  */
	if (cp != cplim) {
		if (i == RTAX_NETMASK + 1 && sa != NULL &&
		    cp - RT_XROUNDUP(sa->sa_len) + sa->sa_len == cplim)
			/*
			 * The last sockaddr was info.rti_info[RTAX_NETMASK].
			 * We accept this for now for the sake of old
			 * binaries or third party softwares.
			 */
			;
		else
			return 1;
	}
	return 0;
}

static int
rt_getlen(int type)
{
#ifndef COMPAT_RTSOCK
	CTASSERT(__alignof(struct ifa_msghdr) >= sizeof(uint64_t));
	CTASSERT(__alignof(struct if_msghdr) >= sizeof(uint64_t));
	CTASSERT(__alignof(struct if_announcemsghdr) >= sizeof(uint64_t));
	CTASSERT(__alignof(struct rt_msghdr) >= sizeof(uint64_t));
#endif

	switch (type) {
	case RTM_ODELADDR:
	case RTM_ONEWADDR:
	case RTM_OCHGADDR:
#ifdef COMPAT_70
		return sizeof(struct ifa_msghdr70);
#else
#ifdef RTSOCK_DEBUG
		printf("%s: unsupported RTM type %d\n", __func__, type);
#endif
		return -1;
#endif
	case RTM_DELADDR:
	case RTM_NEWADDR:
	case RTM_CHGADDR:
		return sizeof(struct ifa_xmsghdr);

	case RTM_OOIFINFO:
#ifdef COMPAT_14
		return sizeof(struct if_msghdr14);
#else
#ifdef RTSOCK_DEBUG
		printf("%s: unsupported RTM type RTM_OOIFINFO\n", __func__);
#endif
		return -1;
#endif
	case RTM_OIFINFO:
#ifdef COMPAT_50
		return sizeof(struct if_msghdr50);
#else
#ifdef RTSOCK_DEBUG
		printf("%s: unsupported RTM type RTM_OIFINFO\n", __func__);
#endif
		return -1;
#endif

	case RTM_IFINFO:
		return sizeof(struct if_xmsghdr);

	case RTM_IFANNOUNCE:
	case RTM_IEEE80211:
		return sizeof(struct if_xannouncemsghdr);

	default:
		return sizeof(struct rt_xmsghdr);
	}
}


struct mbuf *
COMPATNAME(rt_msg1)(int type, struct rt_addrinfo *rtinfo, void *data, int datalen)
{
	struct rt_xmsghdr *rtm;
	struct mbuf *m;
	int i;
	const struct sockaddr *sa;
	int len, dlen;

	m = m_gethdr(M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return m;
	MCLAIM(m, &COMPATNAME(routedomain).dom_mowner);

	if ((len = rt_getlen(type)) == -1)
		goto out;
	if (len > MHLEN + MLEN)
		panic("%s: message too long", __func__);
	else if (len > MHLEN) {
		m->m_next = m_get(M_DONTWAIT, MT_DATA);
		if (m->m_next == NULL)
			goto out;
		MCLAIM(m->m_next, m->m_owner);
		m->m_pkthdr.len = len;
		m->m_len = MHLEN;
		m->m_next->m_len = len - MHLEN;
	} else {
		m->m_pkthdr.len = m->m_len = len;
	}
	m_reset_rcvif(m);
	m_copyback(m, 0, datalen, data);
	if (len > datalen)
		(void)memset(mtod(m, char *) + datalen, 0, len - datalen);
	rtm = mtod(m, struct rt_xmsghdr *);
	for (i = 0; i < RTAX_MAX; i++) {
		if ((sa = rtinfo->rti_info[i]) == NULL)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = RT_XROUNDUP(sa->sa_len);
		m_copyback(m, len, sa->sa_len, sa);
		if (dlen != sa->sa_len) {
			/*
			 * Up to 7 + 1 nul's since roundup is to
			 * sizeof(uint64_t) (8 bytes)
			 */
			m_copyback(m, len + sa->sa_len,
			    dlen - sa->sa_len, "\0\0\0\0\0\0\0");
		}
		len += dlen;
	}
	if (m->m_pkthdr.len != len)
		goto out;
	rtm->rtm_msglen = len;
	rtm->rtm_version = RTM_XVERSION;
	rtm->rtm_type = type;
	return m;
out:
	m_freem(m);
	return NULL;
}

/*
 * rt_msg2
 *
 *	 fills 'cp' or 'w'.w_tmem with the routing socket message and
 *		returns the length of the message in 'lenp'.
 *
 * if walkarg is 0, cp is expected to be 0 or a buffer large enough to hold
 *	the message
 * otherwise walkarg's w_needed is updated and if the user buffer is
 *	specified and w_needed indicates space exists the information is copied
 *	into the temp space (w_tmem). w_tmem is [re]allocated if necessary,
 *	if the allocation fails ENOBUFS is returned.
 */
static int
rt_msg2(int type, struct rt_addrinfo *rtinfo, void *cpv, struct rt_walkarg *w,
	int *lenp)
{
	int i;
	int len, dlen, second_time = 0;
	char *cp0, *cp = cpv;

	rtinfo->rti_addrs = 0;
again:
	if ((len = rt_getlen(type)) == -1)
		return EINVAL;

	if ((cp0 = cp) != NULL)
		cp += len;
	for (i = 0; i < RTAX_MAX; i++) {
		const struct sockaddr *sa;

		if ((sa = rtinfo->rti_info[i]) == NULL)
			continue;
		rtinfo->rti_addrs |= (1 << i);
		dlen = RT_XROUNDUP(sa->sa_len);
		if (cp) {
			int diff = dlen - sa->sa_len;
			(void)memcpy(cp, sa, (size_t)sa->sa_len);
			cp += sa->sa_len;
			if (diff > 0) {
				(void)memset(cp, 0, (size_t)diff);
				cp += diff;
			}
		}
		len += dlen;
	}
	if (cp == NULL && w != NULL && !second_time) {
		struct rt_walkarg *rw = w;

		rw->w_needed += len;
		if (rw->w_needed <= 0 && rw->w_where) {
			if (rw->w_tmemsize < len) {
				if (rw->w_tmem)
					kmem_free(rw->w_tmem, rw->w_tmemsize);
				rw->w_tmem = kmem_alloc(len, KM_SLEEP);
				rw->w_tmemsize = len;
			}
			if (rw->w_tmem) {
				cp = rw->w_tmem;
				second_time = 1;
				goto again;
			} else {
				rw->w_tmemneeded = len;
				return ENOBUFS;
			}
		}
	}
	if (cp) {
		struct rt_xmsghdr *rtm = (struct rt_xmsghdr *)cp0;

		rtm->rtm_version = RTM_XVERSION;
		rtm->rtm_type = type;
		rtm->rtm_msglen = len;
	}
	if (lenp)
		*lenp = len;
	return 0;
}

#ifndef COMPAT_RTSOCK
int
rt_msg3(int type, struct rt_addrinfo *rtinfo, void *cpv, struct rt_walkarg *w,
	int *lenp)
{
	return rt_msg2(type, rtinfo, cpv, w, lenp);
}
#endif

/*
 * This routine is called to generate a message from the routing
 * socket indicating that a redirect has occurred, a routing lookup
 * has failed, or that a protocol has detected timeouts to a particular
 * destination.
 */
void
COMPATNAME(rt_missmsg)(int type, const struct rt_addrinfo *rtinfo, int flags,
    int error)
{
	struct rt_xmsghdr rtm;
	struct mbuf *m;
	const struct sockaddr *sa = rtinfo->rti_info[RTAX_DST];
	struct rt_addrinfo info = *rtinfo;

	COMPATCALL(rt_missmsg, (type, rtinfo, flags, error));
	if (COMPATNAME(route_info).ri_cb.any_count == 0)
		return;
	memset(&rtm, 0, sizeof(rtm));
	rtm.rtm_pid = curproc->p_pid;
	rtm.rtm_flags = RTF_DONE | flags;
	rtm.rtm_errno = error;
	m = COMPATNAME(rt_msg1)(type, &info, &rtm, sizeof(rtm));
	if (m == NULL)
		return;
	mtod(m, struct rt_xmsghdr *)->rtm_addrs = info.rti_addrs;
	COMPATNAME(route_enqueue)(m, sa ? sa->sa_family : 0);
}

/*
 * This routine is called to generate a message from the routing
 * socket indicating that the status of a network interface has changed.
 */
void
COMPATNAME(rt_ifmsg)(struct ifnet *ifp)
{
	struct if_xmsghdr ifm;
	struct mbuf *m;
	struct rt_addrinfo info;

	COMPATCALL(rt_ifmsg, (ifp));
	if (COMPATNAME(route_info).ri_cb.any_count == 0)
		return;
	(void)memset(&info, 0, sizeof(info));
	(void)memset(&ifm, 0, sizeof(ifm));
	ifm.ifm_index = ifp->if_index;
	ifm.ifm_flags = ifp->if_flags;
	ifm.ifm_data = ifp->if_data;
	ifm.ifm_addrs = 0;
	m = COMPATNAME(rt_msg1)(RTM_IFINFO, &info, &ifm, sizeof(ifm));
	if (m == NULL)
		return;
	COMPATNAME(route_enqueue)(m, 0);
#ifdef COMPAT_14
	compat_14_rt_oifmsg(ifp);
#endif
#ifdef COMPAT_50
	compat_50_rt_oifmsg(ifp);
#endif
}

#ifndef COMPAT_RTSOCK
static int
if_addrflags(struct ifaddr *ifa)
{

	switch (ifa->ifa_addr->sa_family) {
#ifdef INET
	case AF_INET:
		return ((struct in_ifaddr *)ifa)->ia4_flags;
#endif
#ifdef INET6
	case AF_INET6:
		return ((struct in6_ifaddr *)ifa)->ia6_flags;
#endif
	default:
		return 0;
	}
}
#endif

/*
 * This is called to generate messages from the routing socket
 * indicating a network interface has had addresses associated with it.
 * if we ever reverse the logic and replace messages TO the routing
 * socket indicate a request to configure interfaces, then it will
 * be unnecessary as the routing socket will automatically generate
 * copies of it.
 */
void
COMPATNAME(rt_newaddrmsg)(int cmd, struct ifaddr *ifa, int error,
    struct rtentry *rt)
{
#define	cmdpass(__cmd, __pass)	(((__cmd) << 2) | (__pass))
	struct rt_addrinfo info;
	const struct sockaddr *sa;
	int pass;
	struct mbuf *m;
	struct ifnet *ifp;
	struct rt_xmsghdr rtm;
	struct ifa_xmsghdr ifam;
	int ncmd;

	KASSERT(ifa != NULL);
	KASSERT(ifa->ifa_addr != NULL);
	ifp = ifa->ifa_ifp;
#ifdef SCTP
	if (cmd == RTM_ADD) {
		sctp_add_ip_address(ifa);
	} else if (cmd == RTM_DELETE) {
		sctp_delete_ip_address(ifa);
	}
#endif

	COMPATCALL(rt_newaddrmsg, (cmd, ifa, error, rt));
	if (COMPATNAME(route_info).ri_cb.any_count == 0)
		return;
	for (pass = 1; pass < 3; pass++) {
		memset(&info, 0, sizeof(info));
		switch (cmdpass(cmd, pass)) {
		case cmdpass(RTM_ADD, 1):
		case cmdpass(RTM_CHANGE, 1):
		case cmdpass(RTM_DELETE, 2):
		case cmdpass(RTM_NEWADDR, 1):
		case cmdpass(RTM_DELADDR, 1):
		case cmdpass(RTM_CHGADDR, 1):
			switch (cmd) {
			case RTM_ADD:
				ncmd = RTM_XNEWADDR;
				break;
			case RTM_DELETE:
				ncmd = RTM_XDELADDR;
				break;
			case RTM_CHANGE:
				ncmd = RTM_XCHGADDR;
				break;
			case RTM_NEWADDR:
				ncmd = RTM_XNEWADDR;
				break;
			case RTM_DELADDR:
				ncmd = RTM_XDELADDR;
				break;
			case RTM_CHGADDR:
				ncmd = RTM_XCHGADDR;
				break;
			default:
				panic("%s: unknown command %d", __func__, cmd);
			}
#ifdef COMPAT_70
			compat_70_rt_newaddrmsg1(ncmd, ifa);
#endif
			info.rti_info[RTAX_IFA] = sa = ifa->ifa_addr;
			KASSERT(ifp->if_dl != NULL);
			info.rti_info[RTAX_IFP] = ifp->if_dl->ifa_addr;
			info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
			info.rti_info[RTAX_BRD] = ifa->ifa_dstaddr;
			memset(&ifam, 0, sizeof(ifam));
			ifam.ifam_index = ifp->if_index;
			ifam.ifam_metric = ifa->ifa_metric;
			ifam.ifam_flags = ifa->ifa_flags;
#ifndef COMPAT_RTSOCK
			ifam.ifam_pid = curproc->p_pid;
			ifam.ifam_addrflags = if_addrflags(ifa);
#endif
			m = COMPATNAME(rt_msg1)(ncmd, &info, &ifam, sizeof(ifam));
			if (m == NULL)
				continue;
			mtod(m, struct ifa_xmsghdr *)->ifam_addrs =
			    info.rti_addrs;
			break;
		case cmdpass(RTM_ADD, 2):
		case cmdpass(RTM_CHANGE, 2):
		case cmdpass(RTM_DELETE, 1):
			if (rt == NULL)
				continue;
			info.rti_info[RTAX_NETMASK] = rt_mask(rt);
			info.rti_info[RTAX_DST] = sa = rt_getkey(rt);
			info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
			memset(&rtm, 0, sizeof(rtm));
			rtm.rtm_pid = curproc->p_pid;
			rtm.rtm_index = ifp->if_index;
			rtm.rtm_flags |= rt->rt_flags;
			rtm.rtm_errno = error;
			m = COMPATNAME(rt_msg1)(cmd, &info, &rtm, sizeof(rtm));
			if (m == NULL)
				continue;
			mtod(m, struct rt_xmsghdr *)->rtm_addrs = info.rti_addrs;
			break;
		default:
			continue;
		}
		KASSERTMSG(m != NULL, "called with wrong command");
		COMPATNAME(route_enqueue)(m, sa ? sa->sa_family : 0);
	}
#undef cmdpass

}

static struct mbuf *
rt_makeifannouncemsg(struct ifnet *ifp, int type, int what,
    struct rt_addrinfo *info)
{
	struct if_xannouncemsghdr ifan;

	memset(info, 0, sizeof(*info));
	memset(&ifan, 0, sizeof(ifan));
	ifan.ifan_index = ifp->if_index;
	strlcpy(ifan.ifan_name, ifp->if_xname, sizeof(ifan.ifan_name));
	ifan.ifan_what = what;
	return COMPATNAME(rt_msg1)(type, info, &ifan, sizeof(ifan));
}

/*
 * This is called to generate routing socket messages indicating
 * network interface arrival and departure.
 */
void
COMPATNAME(rt_ifannouncemsg)(struct ifnet *ifp, int what)
{
	struct mbuf *m;
	struct rt_addrinfo info;

	COMPATCALL(rt_ifannouncemsg, (ifp, what));
	if (COMPATNAME(route_info).ri_cb.any_count == 0)
		return;
	m = rt_makeifannouncemsg(ifp, RTM_IFANNOUNCE, what, &info);
	if (m == NULL)
		return;
	COMPATNAME(route_enqueue)(m, 0);
}

/*
 * This is called to generate routing socket messages indicating
 * IEEE80211 wireless events.
 * XXX we piggyback on the RTM_IFANNOUNCE msg format in a clumsy way.
 */
void
COMPATNAME(rt_ieee80211msg)(struct ifnet *ifp, int what, void *data,
	size_t data_len)
{
	struct mbuf *m;
	struct rt_addrinfo info;

	COMPATCALL(rt_ieee80211msg, (ifp, what, data, data_len));
	if (COMPATNAME(route_info).ri_cb.any_count == 0)
		return;
	m = rt_makeifannouncemsg(ifp, RTM_IEEE80211, what, &info);
	if (m == NULL)
		return;
	/*
	 * Append the ieee80211 data.  Try to stick it in the
	 * mbuf containing the ifannounce msg; otherwise allocate
	 * a new mbuf and append.
	 *
	 * NB: we assume m is a single mbuf.
	 */
	if (data_len > M_TRAILINGSPACE(m)) {
		struct mbuf *n = m_get(M_NOWAIT, MT_DATA);
		if (n == NULL) {
			m_freem(m);
			return;
		}
		(void)memcpy(mtod(n, void *), data, data_len);
		n->m_len = data_len;
		m->m_next = n;
	} else if (data_len > 0) {
		(void)memcpy(mtod(m, uint8_t *) + m->m_len, data, data_len);
		m->m_len += data_len;
	}
	if (m->m_flags & M_PKTHDR)
		m->m_pkthdr.len += data_len;
	mtod(m, struct if_xannouncemsghdr *)->ifan_msglen += data_len;
	COMPATNAME(route_enqueue)(m, 0);
}

#ifndef COMPAT_RTSOCK
/*
 * Send a routing message as mimicing that a cloned route is added.
 */
void
rt_clonedmsg(const struct sockaddr *dst, const struct ifnet *ifp,
    const struct rtentry *rt)
{
	struct rt_addrinfo info;
	/* Mimic flags exactly */
#define RTF_LLINFO	0x400
#define RTF_CLONED	0x2000
	int flags = RTF_UP | RTF_HOST | RTF_DONE | RTF_LLINFO | RTF_CLONED;
	union {
		struct sockaddr sa;
		struct sockaddr_storage ss;
		struct sockaddr_dl sdl;
	} u;
	uint8_t namelen = strlen(ifp->if_xname);
	uint8_t addrlen = ifp->if_addrlen;

	if (rt == NULL)
		return; /* XXX */

	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	sockaddr_dl_init(&u.sdl, sizeof(u.ss), ifp->if_index, ifp->if_type,
	    NULL, namelen, NULL, addrlen);
	info.rti_info[RTAX_GATEWAY] = &u.sa;

	rt_missmsg(RTM_ADD, &info, flags, 0);
#undef RTF_LLINFO
#undef RTF_CLONED
}
#endif /* COMPAT_RTSOCK */

/*
 * This is used in dumping the kernel table via sysctl().
 */
static int
sysctl_dumpentry(struct rtentry *rt, void *v)
{
	struct rt_walkarg *w = v;
	int error = 0, size;
	struct rt_addrinfo info;

	if (w->w_op == NET_RT_FLAGS && !(rt->rt_flags & w->w_arg))
		return 0;
	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rt_getkey(rt);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	info.rti_info[RTAX_TAG] = rt_gettag(rt);
	if (rt->rt_ifp) {
		const struct ifaddr *rtifa;
		info.rti_info[RTAX_IFP] = rt->rt_ifp->if_dl->ifa_addr;
		/* rtifa used to be simply rt->rt_ifa.  If rt->rt_ifa != NULL,
		 * then rt_get_ifa() != NULL.  So this ought to still be safe.
		 * --dyoung
		 */
		rtifa = rt_get_ifa(rt);
		info.rti_info[RTAX_IFA] = rtifa->ifa_addr;
		if (rt->rt_ifp->if_flags & IFF_POINTOPOINT)
			info.rti_info[RTAX_BRD] = rtifa->ifa_dstaddr;
	}
	if ((error = rt_msg2(RTM_GET, &info, 0, w, &size)))
		return error;
	if (w->w_where && w->w_tmem && w->w_needed <= 0) {
		struct rt_xmsghdr *rtm = (struct rt_xmsghdr *)w->w_tmem;

		rtm->rtm_flags = rt->rt_flags;
		rtm->rtm_use = rt->rt_use;
		rtm_setmetrics(rt, rtm);
		KASSERT(rt->rt_ifp != NULL);
		rtm->rtm_index = rt->rt_ifp->if_index;
		rtm->rtm_errno = rtm->rtm_pid = rtm->rtm_seq = 0;
		rtm->rtm_addrs = info.rti_addrs;
		if ((error = copyout(rtm, w->w_where, size)) != 0)
			w->w_where = NULL;
		else
			w->w_where = (char *)w->w_where + size;
	}
	return error;
}

static int
sysctl_iflist_if(struct ifnet *ifp, struct rt_walkarg *w,
    struct rt_addrinfo *info, size_t len)
{
	struct if_xmsghdr *ifm;
	int error;

	ifm = (struct if_xmsghdr *)w->w_tmem;
	ifm->ifm_index = ifp->if_index;
	ifm->ifm_flags = ifp->if_flags;
	ifm->ifm_data = ifp->if_data;
	ifm->ifm_addrs = info->rti_addrs;
	if ((error = copyout(ifm, w->w_where, len)) == 0)
		w->w_where = (char *)w->w_where + len;
	return error;
}

static int
sysctl_iflist_addr(struct rt_walkarg *w, struct ifaddr *ifa,
     struct rt_addrinfo *info)
{
	int len, error;

	if ((error = rt_msg2(RTM_XNEWADDR, info, 0, w, &len)))
		return error;
	if (w->w_where && w->w_tmem && w->w_needed <= 0) {
		struct ifa_xmsghdr *ifam;

		ifam = (struct ifa_xmsghdr *)w->w_tmem;
		ifam->ifam_index = ifa->ifa_ifp->if_index;
		ifam->ifam_flags = ifa->ifa_flags;
		ifam->ifam_metric = ifa->ifa_metric;
		ifam->ifam_addrs = info->rti_addrs;
#ifndef COMPAT_RTSOCK
		ifam->ifam_pid = 0;
		ifam->ifam_addrflags = if_addrflags(ifa);
#endif
		if ((error = copyout(w->w_tmem, w->w_where, len)) == 0)
			w->w_where = (char *)w->w_where + len;
	}
	return error;
}

static int
sysctl_iflist(int af, struct rt_walkarg *w, int type)
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct	rt_addrinfo info;
	int	cmd, len, error = 0;
	int	(*iflist_if)(struct ifnet *, struct rt_walkarg *,
			     struct rt_addrinfo *, size_t);
	int	(*iflist_addr)(struct rt_walkarg *, struct ifaddr *,
			       struct rt_addrinfo *);
	int s;
	struct psref psref;
	int bound;

	switch (type) {
	case NET_RT_IFLIST:
		cmd = RTM_IFINFO;
		iflist_if = sysctl_iflist_if;
		iflist_addr = sysctl_iflist_addr;
		break;
#ifdef COMPAT_14
	case NET_RT_OOOIFLIST:
		cmd = RTM_OOIFINFO;
		iflist_if = compat_14_iflist;
		iflist_addr = compat_70_iflist_addr;
		break;
#endif
#ifdef COMPAT_50
	case NET_RT_OOIFLIST:
		cmd = RTM_OIFINFO;
		iflist_if = compat_50_iflist;
		iflist_addr = compat_70_iflist_addr;
		break;
#endif
#ifdef COMPAT_70
	case NET_RT_OIFLIST:
		cmd = RTM_IFINFO;
		iflist_if = sysctl_iflist_if;
		iflist_addr = compat_70_iflist_addr;
		break;
#endif
	default:
#ifdef RTSOCK_DEBUG
		printf("%s: unsupported IFLIST type %d\n", __func__, type);
#endif
		return EINVAL;
	}

	memset(&info, 0, sizeof(info));

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		int _s;
		if (w->w_arg && w->w_arg != ifp->if_index)
			continue;
		if (IFADDR_READER_EMPTY(ifp))
			continue;

		if_acquire(ifp, &psref);
		pserialize_read_exit(s);

		info.rti_info[RTAX_IFP] = ifp->if_dl->ifa_addr;
		if ((error = rt_msg2(cmd, &info, NULL, w, &len)) != 0)
			goto release_exit;
		info.rti_info[RTAX_IFP] = NULL;
		if (w->w_where && w->w_tmem && w->w_needed <= 0) {
			if ((error = iflist_if(ifp, w, &info, len)) != 0)
				goto release_exit;
		}
		_s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			struct psref _psref;
			if (af && af != ifa->ifa_addr->sa_family)
				continue;
			ifa_acquire(ifa, &_psref);
			pserialize_read_exit(_s);

			info.rti_info[RTAX_IFA] = ifa->ifa_addr;
			info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
			info.rti_info[RTAX_BRD] = ifa->ifa_dstaddr;
			error = iflist_addr(w, ifa, &info);

			_s = pserialize_read_enter();
			ifa_release(ifa, &_psref);
			if (error != 0) {
				pserialize_read_exit(_s);
				goto release_exit;
			}
		}
		pserialize_read_exit(_s);
		info.rti_info[RTAX_IFA] = info.rti_info[RTAX_NETMASK] =
		    info.rti_info[RTAX_BRD] = NULL;

		s = pserialize_read_enter();
		if_release(ifp, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	return 0;

release_exit:
	if_release(ifp, &psref);
	curlwp_bindx(bound);
	return error;
}

static int
sysctl_rtable(SYSCTLFN_ARGS)
{
	void 	*where = oldp;
	size_t	*given = oldlenp;
	int	i, s, error = EINVAL;
	u_char  af;
	struct	rt_walkarg w;

	if (namelen == 1 && name[0] == CTL_QUERY)
		return sysctl_query(SYSCTLFN_CALL(rnode));

	if (newp)
		return EPERM;
	if (namelen != 3)
		return EINVAL;
	af = name[0];
	w.w_tmemneeded = 0;
	w.w_tmemsize = 0;
	w.w_tmem = NULL;
again:
	/* we may return here if a later [re]alloc of the t_mem buffer fails */
	if (w.w_tmemneeded) {
		w.w_tmem = kmem_alloc(w.w_tmemneeded, KM_SLEEP);
		w.w_tmemsize = w.w_tmemneeded;
		w.w_tmemneeded = 0;
	}
	w.w_op = name[1];
	w.w_arg = name[2];
	w.w_given = *given;
	w.w_needed = 0 - w.w_given;
	w.w_where = where;

	s = splsoftnet();
	switch (w.w_op) {

	case NET_RT_DUMP:
	case NET_RT_FLAGS:
#if defined(INET) || defined(INET6)
		/*
		 * take care of llinfo entries, the caller must
		 * specify an AF
		 */
		if (w.w_op == NET_RT_FLAGS &&
		    (w.w_arg == 0 || w.w_arg & RTF_LLDATA)) {
			if (af != 0)
				error = lltable_sysctl_dump(af, &w);
			else
				error = EINVAL;
			break;
		}
#endif

		for (i = 1; i <= AF_MAX; i++) {
			if (af == 0 || af == i) {
				error = rt_walktree(i, sysctl_dumpentry, &w);
				if (error != 0)
					break;
#if defined(INET) || defined(INET6)
				/*
				 * Return ARP/NDP entries too for
				 * backward compatibility.
				 */
				error = lltable_sysctl_dump(i, &w);
				if (error != 0)
					break;
#endif
			}
		}
		break;

#ifdef COMPAT_14
	case NET_RT_OOOIFLIST:
		error = sysctl_iflist(af, &w, w.w_op);
		break;
#endif
#ifdef COMPAT_50
	case NET_RT_OOIFLIST:
		error = sysctl_iflist(af, &w, w.w_op);
		break;
#endif
#ifdef COMPAT_70
	case NET_RT_OIFLIST:
		error = sysctl_iflist(af, &w, w.w_op);
		break;
#endif
	case NET_RT_IFLIST:
		error = sysctl_iflist(af, &w, w.w_op);
		break;
	}
	splx(s);

	/* check to see if we couldn't allocate memory with NOWAIT */
	if (error == ENOBUFS && w.w_tmem == 0 && w.w_tmemneeded)
		goto again;

	if (w.w_tmem)
		kmem_free(w.w_tmem, w.w_tmemsize);
	w.w_needed += w.w_given;
	if (where) {
		*given = (char *)w.w_where - (char *)where;
		if (*given < w.w_needed)
			return ENOMEM;
	} else {
		*given = (11 * w.w_needed) / 10;
	}
	return error;
}

/*
 * Routing message software interrupt routine
 */
static void
COMPATNAME(route_intr)(void *cookie)
{
	struct sockproto proto = { .sp_family = PF_XROUTE, };
	struct route_info * const ri = &COMPATNAME(route_info);
	struct mbuf *m;

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	for (;;) {
		IFQ_LOCK(&ri->ri_intrq);
		IF_DEQUEUE(&ri->ri_intrq, m);
		IFQ_UNLOCK(&ri->ri_intrq);
		if (m == NULL)
			break;
		proto.sp_protocol = M_GETCTX(m, uintptr_t);
#ifdef NET_MPSAFE
		mutex_enter(rt_so_mtx);
#endif
		raw_input(m, &proto, &ri->ri_src, &ri->ri_dst, &rt_rawcb);
#ifdef NET_MPSAFE
		mutex_exit(rt_so_mtx);
#endif
	}
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

/*
 * Enqueue a message to the software interrupt routine.
 */
void
COMPATNAME(route_enqueue)(struct mbuf *m, int family)
{
	struct route_info * const ri = &COMPATNAME(route_info);
	int wasempty;

	IFQ_LOCK(&ri->ri_intrq);
	if (IF_QFULL(&ri->ri_intrq)) {
		printf("%s: queue full, dropped message\n", __func__);
		IF_DROP(&ri->ri_intrq);
		IFQ_UNLOCK(&ri->ri_intrq);
		m_freem(m);
	} else {
		wasempty = IF_IS_EMPTY(&ri->ri_intrq);
		M_SETCTX(m, (uintptr_t)family);
		IF_ENQUEUE(&ri->ri_intrq, m);
		IFQ_UNLOCK(&ri->ri_intrq);
		if (wasempty) {
			kpreempt_disable();
			softint_schedule(ri->ri_sih);
			kpreempt_enable();
		}
	}
}

static void
COMPATNAME(route_init)(void)
{
	struct route_info * const ri = &COMPATNAME(route_info);

#ifndef COMPAT_RTSOCK
	rt_init();
#endif
#ifdef NET_MPSAFE
	rt_so_mtx = mutex_obj_alloc(MUTEX_DEFAULT, IPL_NONE);

	cv_init(&rt_update_cv, "rtsock_cv");
#endif

	sysctl_net_route_setup(NULL);
	ri->ri_intrq.ifq_maxlen = ri->ri_maxqlen;
	ri->ri_sih = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    COMPATNAME(route_intr), NULL);
	IFQ_LOCK_INIT(&ri->ri_intrq);
}

/*
 * Definitions of protocols supported in the ROUTE domain.
 */
#ifndef COMPAT_RTSOCK
PR_WRAP_USRREQS(route);
#else
PR_WRAP_USRREQS(compat_50_route);
#endif

static const struct pr_usrreqs route_usrreqs = {
	.pr_attach	= COMPATNAME(route_attach_wrapper),
	.pr_detach	= COMPATNAME(route_detach_wrapper),
	.pr_accept	= COMPATNAME(route_accept_wrapper),
	.pr_bind	= COMPATNAME(route_bind_wrapper),
	.pr_listen	= COMPATNAME(route_listen_wrapper),
	.pr_connect	= COMPATNAME(route_connect_wrapper),
	.pr_connect2	= COMPATNAME(route_connect2_wrapper),
	.pr_disconnect	= COMPATNAME(route_disconnect_wrapper),
	.pr_shutdown	= COMPATNAME(route_shutdown_wrapper),
	.pr_abort	= COMPATNAME(route_abort_wrapper),
	.pr_ioctl	= COMPATNAME(route_ioctl_wrapper),
	.pr_stat	= COMPATNAME(route_stat_wrapper),
	.pr_peeraddr	= COMPATNAME(route_peeraddr_wrapper),
	.pr_sockaddr	= COMPATNAME(route_sockaddr_wrapper),
	.pr_rcvd	= COMPATNAME(route_rcvd_wrapper),
	.pr_recvoob	= COMPATNAME(route_recvoob_wrapper),
	.pr_send	= COMPATNAME(route_send_wrapper),
	.pr_sendoob	= COMPATNAME(route_sendoob_wrapper),
	.pr_purgeif	= COMPATNAME(route_purgeif_wrapper),
};

static const struct protosw COMPATNAME(route_protosw)[] = {
	{
		.pr_type = SOCK_RAW,
		.pr_domain = &COMPATNAME(routedomain),
		.pr_flags = PR_ATOMIC|PR_ADDR,
		.pr_ctlinput = raw_ctlinput,
		.pr_ctloutput = route_ctloutput,
		.pr_usrreqs = &route_usrreqs,
		.pr_init = rt_pr_init,
	},
};

struct domain COMPATNAME(routedomain) = {
	.dom_family = PF_XROUTE,
	.dom_name = DOMAINNAME,
	.dom_init = COMPATNAME(route_init),
	.dom_protosw = COMPATNAME(route_protosw),
	.dom_protoswNPROTOSW =
	    &COMPATNAME(route_protosw)[__arraycount(COMPATNAME(route_protosw))],
};

static void
sysctl_net_route_setup(struct sysctllog **clog)
{
	const struct sysctlnode *rnode = NULL;

	sysctl_createv(clog, 0, NULL, &rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, DOMAINNAME,
		       SYSCTL_DESCR("PF_ROUTE information"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_XROUTE, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "rtable",
		       SYSCTL_DESCR("Routing table information"),
		       sysctl_rtable, 0, NULL, 0,
		       CTL_NET, PF_XROUTE, 0 /* any protocol */, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("Routing statistics"),
		       NULL, 0, &rtstat, sizeof(rtstat),
		       CTL_CREATE, CTL_EOL);
}
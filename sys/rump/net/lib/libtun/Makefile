#	$NetBSD$
#

.PATH:	${.CURDIR}/../../../../net

LIB=	rumpnet_tun
COMMENT=/dev/tun network tunneling over tty

IOCONF=	TUN.ioconf
SRCS=	if_tun.c

SRCS+=	tun_component.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

#	$NetBSD: Makefile.inc,v 1.1 2013/07/18 22:08:59 matt Exp $
#
#	etc.evbcf/Makefile.inc -- evbcf-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

# Not ready yet
.if 0
KERNEL_SETS=		TWR54418

BUILD_KERNELS=		TWR54418_INSTALL
.endif

snap_md_post:
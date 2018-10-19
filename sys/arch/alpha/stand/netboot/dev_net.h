/*	$NetBSD: dev_net.h,v 1.3 2002/11/09 06:34:38 thorpej Exp $	*/


int	net_open(struct open_file *, ...);
int	net_close(struct open_file *);
int	net_ioctl(struct open_file *, u_long, void *);
int	net_strategy(void *, int , daddr_t , size_t, void *, size_t *);

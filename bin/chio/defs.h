/* $NetBSD: defs.h,v 1.5 2002/03/09 19:32:03 wiz Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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

#ifndef _DEFS_H_
#define _DEFS_H_

struct element_type {
	const char	*et_name;	/* name; i.e. "picker, "slot", etc. */
	int		et_type;	/* type number */
};

struct changer_command {
	const char	*cc_name;	/* command name */
	const char	*cc_args;	/* usage string */
					/* command handler */
	int		(*cc_handler)(const char *, int, char **);
};

struct special_word {
	const char	*sw_name;	/* special word */
	int		sw_value;	/* token value */
};

/* sw_value */
#define	SW_INVERT	1	/* set "invert media" flag */
#define	SW_INVERT1	2	/* set "invert media 1" flag */
#define	SW_INVERT2	3	/* set "invert media 2" flag */
#define	SW_VOLTAGS	4	/* set "retrieve voltags" flags */

/* Environment variable to check for default changer. */
#define	CHANGER_ENV_VAR		"CHANGER"

#endif /* !_DEFS_H_ */

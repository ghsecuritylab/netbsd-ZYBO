/* $NetBSD$ */

/*
 * Automatically generated by ./genheaders.sh on Wed May 16 14:39:02 CEST 2018
 * Do not modify directly!
 */
#ifndef _USERMODE_MCONTEXT_H
#define _USERMODE_MCONTEXT_H

#if defined(__i386__)
#include "../../i386/include/mcontext.h"
#elif defined(__x86_64__)
#include "../../amd64/include/mcontext.h"
#elif defined(__arm__)
#include "../../arm/include/mcontext.h"
#else
#error port me
#endif

#endif
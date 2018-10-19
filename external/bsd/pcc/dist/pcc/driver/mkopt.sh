#	Id	
#	$NetBSD$

#-
# Copyright (c) 2014 Iain Hibbert.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# emit a NULL terminated array of quoted strings
array()
{
	echo -n "{"
	while
		test $# -gt 0
	do
		echo -n " \"$1\","
		shift
	done
	echo -n " NULL }"
}

cat << EOF
/**************************************
 * This file is generated by mkopt.sh *
 **************************************/
	
#include "driver.h"

const char prog_cpp[] = "${PROG_CPP:-cpp}";
const char prog_ccom[] = "${PROG_CCOM:-ccom}";
const char prog_cxxcom[] = "${PROG_CXXCOM:-cxxcom}";
const char prog_fcom[] = "${PROG_FCOM:-fcom}";
const char prog_asm[] = "${PROG_ASM:-as}";
const char prog_ld[] = "${PROG_LD:-ld}";

const char dir_libexec[] = "${LIBEXEC:-/usr/libexec}";

const char dir_stdinc[] = "${STDINC:=/usr/include}";
const char dir_pccinc[] = "${PCCINC:-${STDINC}/pcc}";
const char dir_pccinc[] = "${PCCINC:-${STDINC}/pcc}";
const char dir_ftninc[] = "${FTNINC:-${STDINC}/ftn}";

const char dir_stdlib[] = "${STDLIB:=/usr/lib}";
const char dir_pcclib[] = "${PCCLIB:-${STDLIB}/pcc}";
const char dir_pxxlib[] = "${PXXLIB:-${STDLIB}/p++}";
const char dir_ftnlib[] = "${FTNLIB:-${STDLIB}/ftn}";

const struct target *target = &targ_${TARGET};

const char *cc_names[] = $(array ${CC_NAMES:-cc pcc});
const char *cpp_names[] = $(array ${CPP_NAMES:-cpp pcpp});
const char *cxx_names[] = $(array ${CXX_NAMES:-c++ p++});
const char *ftn_names[] = $(array ${FTN_NAMES:-f77 fortran p77 pfortran});

/**************************************
 * This file is generated by mkopt.sh *
 **************************************/
EOF
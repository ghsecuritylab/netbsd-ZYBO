# $NetBSD$

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils

.PATH:		${SRCDIR}/utils

TESTS_CXX=	auto_array_test \
		datetime_test \
		env_test \
		memory_test \
		optional_test \
		passwd_test \
		sanity_test \
		stream_test \
		units_test

TESTS_SUBDIRS+=	cmdline config format fs logging process signals sqlite text

.include <bsd.test.mk>

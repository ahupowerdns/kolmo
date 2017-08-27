AC_INIT([kolmo], 0.1)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign dist-bzip2 no-dist-gzip tar-ustar -Wno-portability subdir-objects parallel-tests 1.11])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([ws.cc])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_CANONICAL_HOST
: ${CFLAGS="-Wall -g -O2"}
: ${CXXFLAGS="-Wall -g -O2"}

AC_PROG_CXX
AS_IF([test "x$CXX" = "xno" || test "x$CXX:x$GXX" = "xg++:x"],
  AC_MSG_ERROR([no C++ compiler found])
)
AX_CXX_COMPILE_STDCXX([14],ext,mandatory)
AC_PROG_LIBTOOL
BOOST_REQUIRE([1.35])
BOOST_SYSTEM([mt])
#AC_LANG([C++])

AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_DEFINE([_GNU_SOURCE], [1],
  [Define _GNU_SOURCE so that we get all necessary prototypes]
)

AC_SUBST(LIBS)

AC_SUBST([AM_CPPFLAGS],
  ["AS_ESCAPE([-I$(top_builddir) -I$(top_srcdir)]) $THREADFLAGS"]
)

AC_SUBST([PROGRAM_LDFLAGS])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

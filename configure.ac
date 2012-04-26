#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([fuse-google-drive], [0])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([dist-bzip2 no-dist-gzip tar-ustar foreign])
AC_CONFIG_SRCDIR([.])


# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([fuse], [fuse_main],
	     [
	      FUSE_LIBS=`pkg-config fuse --libs`
	      FUSE_CFLAGS=`pkg-config fuse --cflags`
	      AC_SUBST([FUSE_LIBS])
	      AC_SUBST([FUSE_CFLAGS])
	     ],
	     [AC_MSG_ERROR([FUSE library is missing])],
	     )
PKG_CHECK_MODULES([curl], [libcurl])
PKG_CHECK_MODULES([json], [json])

# Checks for header files.
CFLAGS="$CFLAGS $FUSE_CFLAGS"
AC_CHECK_HEADER([fuse.h],,[AC_MSG_ERROR([<fuse.h> header not found])], [AC_INCLUDES_DEFAULT])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([utime])

AC_CONFIG_FILES([
Makefile
])

AC_OUTPUT

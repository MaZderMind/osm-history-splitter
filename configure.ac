dnl Process this file with autoconf to produce a configure script.

AC_INIT(splitter.cpp)
dnl Every other copy of the package version number gets its value from here
AM_INIT_AUTOMAKE([osm_history_splitter],
                 [0.1.0]
                 [https://github.com/MaZderMind/osm-history-splitter/issues],
                 [osm_history_splitter-0.1.0],
                 [https://github.com/MaZderMind/osm-history-splitter])

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AM_CONFIG_HEADER(config.h)

AC_SUBST(VERSION)

ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

AC_CANONICAL_HOST

dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX

dnl Checks for libraries.
AC_LANG(C++)

AC_CHECK_HEADER([osmpbf/osmpbf.h],[],
        [AC_MSG_ERROR(
        [Unable to find the osmpbf headers, you might need to install libosmpbf-dev.]
        )])

AX_BOOST_BASE([1.43],,[AC_MSG_ERROR(
        [Boost libraries are required. Please install libboost-dev.]
        )])

AX_BOOST_BASE
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_DATE_TIME
AX_BOOST_THREAD
AX_BOOST_IOSTREAMS

PKG_CHECK_MODULES([PROTOBUF_LITE], "protobuf-lite")
AC_SUBST([PROTOBUF_LITE_CFLAGS])
AC_SUBST([PROTOBUF_LITE_LIBS])

PKG_CHECK_MODULES([PROTOBUF], "protobuf")
AC_SUBST([PROTOBUF_CFLAGS])
AC_SUBST([PROTOBUF_LIBS])


dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h sys/param.h sys/time.h time.h sys/mkdev.h sys/sysmacros.h string.h memory.h fcntl.h dirent.h sys/ndir.h ndir.h alloca.h locale.h )

dnl Additional macro definitions are in here
AC_CONFIG_MACRO_DIR([m4])

AM_PATH_XML2(2.4.0)

dnl Check for Geos library
AX_LIB_GEOS
if test "x$GEOS_VERSION" = "x"
then
AC_MSG_ERROR([geos library not found]);
fi

#jm_CHECK_TYPE_STRUCT_UTIMBUF
AC_HEADER_MAJOR
AC_FUNC_ALLOCA
AC_STRUCT_TM
AC_STRUCT_ST_BLOCKS
AC_FUNC_CLOSEDIR_VOID
AC_CHECK_FUNCS(mkfifo)
AC_CHECK_FUNC(mknod)

dnl Checks for typedefs, structures, and compiler characteristics.

dnl Checks for library functions.

AC_OUTPUT(Makefile osm_history_splitter.lsm osm_history_splitter.spec)

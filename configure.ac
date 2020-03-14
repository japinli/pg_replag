dnl --------------------------------------------------------------------------
dnl
dnl configure.ac
dnl     Process this file to generate configure
dnl
dnl --------------------------------------------------------------------------

AC_INIT([pg_replag], [0.1], [japinli@hotmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC

AC_CHECK_PROG([has_pg_config], [pg_config], [yes])
AS_IF([test "x$has_pg_config" = xyes],
      [],
      [AC_MSG_ERROR([
------------------------------------------------------------------------------
 Cannot find `pg_config`
------------------------------------------------------------------------------
      ])])

# Supply a numeric version string for use
# awk -F is a regex on some platforms, and not on others, so make "." a tab
[PG_VERSION_NUM="`pg_config --version | cut -d ' ' -f 2 |
 sed 's/[A-Za-z].*$//' | tr '.' '        ' |
 awk '{ if (NF == 2) printf "%d%04d", $1, $2; else printf "%d%02d%02d", $1, $2, $3 }'`"]
AC_DEFINE_UNQUOTED(PG_VERSION_NUM, $PG_VERSION_NUM, [PostgreSQL version as a number])

[PG_LIBDIR="`pg_config --libdir`"]
[PG_INCLUDEDIR="`pg_config --includedir`"]
AC_SUBST(PG_LIBDIR)
AC_SUBST(PG_INCLUDEDIR)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

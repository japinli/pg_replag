dist_data_DATA = README.md autogen.sh
include_HEADERS = pg10.h pg9x.h

bin_PROGRAMS = pg_replag
pg_replag_SOURCES = replag.c
pg_replag_LDFLAGS = -L@PG_LIBDIR@
pg_replag_LDADD = -lpq

AM_CPPFLAGS = -I@PG_INCLUDEDIR@

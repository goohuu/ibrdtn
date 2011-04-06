## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

api_SOURCES = \
		ExtendedApiServer.h \
		ExtendedApiServer.cpp
				
noinst_LTLIBRARIES = libapi.la
libapi_la_SOURCES= $(api_SOURCES)

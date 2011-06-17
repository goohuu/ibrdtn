## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

api_SOURCES = \
		ExtendedApiServer.h \
		ExtendedApiServer.cpp \
		ApiServer.h \
		ApiServer.cpp \
		ClientHandler.cpp \
		ClientHandler.h \
		Registration.h \
		Registration.cpp \
		ExtendedApiConnection.h \
		ExtendedApiConnection.cpp \
		PlainSerializer.h \
		PlainSerializer.cpp
				
noinst_LTLIBRARIES = libapi.la
libapi_la_SOURCES= $(api_SOURCES)

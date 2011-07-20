## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

api_SOURCES = \
		ApiServer.h \
		ApiServer.cpp \
		ClientHandler.cpp \
		ClientHandler.h \
		Registration.h \
		Registration.cpp \
		BinaryStreamClient.h \
		BinaryStreamClient.cpp
		
noinst_LTLIBRARIES = libapi.la
libapi_la_SOURCES= $(api_SOURCES)

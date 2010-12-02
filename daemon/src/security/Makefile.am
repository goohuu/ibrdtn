## sub directory

AM_CPPFLAGS = -I$(top_srcdir) @ibrdtn_CFLAGS@
AM_LDFLAGS = @ibrdtn_LIBS@

security_SOURCES =	\
	SecurityManager.h \
	SecurityManager.cpp \
	KeyServerWorker.h \
	KeyServerWorker.cpp \
	RuleServerWorker.h \
	RuleServerWorker.cpp \
	SecurityKeyManager.h \
	SecurityKeyManager.cpp \
	SecurityRule.h \
	SecurityRule.cpp
				
noinst_LTLIBRARIES = libsecurity.la
libsecurity_la_SOURCES= $(security_SOURCES)

## Process this file with automake to generate Makefile.in

INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libvarconf.la

libvarconf_la_LDFLAGS = @VARCONF_VERSION_INFO@ $(SIGC_LIBS)

libvarconf_la_SOURCES = \
    config.cpp parse_error.cpp variable.cpp \
    dynbase.cpp dyntypes.cpp dyncmp.cpp

libvarconfincludedir = $(includedir)/varconf-0.5/varconf
libvarconfinclude_HEADERS = \
    Config.h parse_error.h varconf.h variable.h \
    dynbase.h dyntypes.h dyncmp.h dynvar.h

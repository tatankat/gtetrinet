SUBDIRS = images

localedir = $(datadir)/locale

gtetrinet_LDADD = $(glib_LIBS) $(gtk_LIBS) $(libcanberra_LIBS) $(libsoup_LIBS)

AM_CPPFLAGS = \
	-DGTETRINET_DATA=\"$(pkgdatadir)\"                      \
	-DGTETPIXMAPSDIR=\""$(datadir)/pixmaps/gtetrinet"\"     \
	-DLOCALEDIR=\"$(localedir)\"                            \
	-DPIXMAPSDIR=\""$(datadir)/pixmaps"\"                   \
	-DGSETTINGSSCHEMADIR=\"$(gsettingsschemadir)\"          \
	$(glib_CFLAGS) $(gtk_CFLAGS) $(canberragtk_CFLAGS) $(libsoup_CFLAGS)

gamesdir = $(prefix)/games
games_PROGRAMS = gtetrinet

gtetrinet_SOURCES = \
	client.c \
	client.h \
	commands.c \
	commands.h \
	config.c \
	config.h \
	dialogs.c \
	dialogs.h \
	fields.c \
	fields.h \
	gtetrinet.c \
	gtetrinet.h \
	misc.c \
	misc.h \
	partyline.c \
	partyline.h \
	sound.c \
	sound.h \
	tetrinet.c \
	tetrinet.h \
	tetris.c \
	tetris.h \
	winlist.c \
	winlist.h \
	resources.c

BUILT_SOURCES = resources.c

GLIB_COMPILE_RESOURCES=glib-compile-resources
resources.c: gtetrinet.gresource.xml.in partyline.ui field.ui fields.ui winlist.ui connectdialog.ui
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) $(srcdir)/gtetrinet.gresource.xml.in \
		--target=$@ --sourcedir=$(srcdir) --generate-source

-include $(top_srcdir)/git.mk

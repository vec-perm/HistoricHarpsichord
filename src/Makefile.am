bin_PROGRAMS = historicHarpsichord
dist_pkgdata_DATA = instruments.xml
historicHarpsichord_SOURCES = \
  audio/temperament.c       \
  audio/portmidiutil.c \
  core/prefops.c \
  audio/temperament.h       \
  audio/portmidiutil.h \
  core/prefops.h \
  audio/audiointerface.c \
  audio/ringbuffer.c   \
  core/utils.c \
  audio/audiointerface.h \
  audio/ringbuffer.h   \
  core/utils.h \
  audio/portaudioutil.c    \
  core/binreloc.c      \
  audio/portaudioutil.h    \
  core/binreloc.h      \
  audio/eventqueue.c     \
  audio/midi.c        \
  audio/eventqueue.h     \
  audio/midi.h        \
  core/main.c          \
  ui/prefdialog.c
  
nodist_historicHarpsichord_SOURCES = pathconfig.h

noinst_LIBRARIES = libaudiobackend.a
libaudiobackend_a_CFLAGS = -W -Wall -Wno-unused-parameter $(PLATFORM_CFLAGS) 
libaudiobackend_a_SOURCES = \
  audio/audiointerface.c \
  audio/audiointerface.h \
  audio/dummybackend.c \
  audio/dummybackend.h \
  audio/eventqueue.c \
  audio/eventqueue.h \
  audio/fluid.c \
  audio/fluid.h \
  audio/portaudiobackend.c \
  audio/portaudiobackend.h \
  audio/portaudioutil.c \
  audio/portaudioutil.h \
  audio/portmidibackend.c \
  audio/portmidibackend.h \
  audio/portmidiutil.c \
  audio/portmidiutil.h \
  audio/ringbuffer.c \
  audio/ringbuffer.h

AM_CPPFLAGS = \
   $(BINRELOC_CFLAGS) \
   $(PORTMIDI_INCLUDE) \
  -I$(top_srcdir)/intl \
  -I$(top_srcdir)/include \
  -I$(top_srcdir)/libs/libsffile \
  -I$(top_srcdir)/pixmaps \
  -DPREFIX=\"$(prefix)\" \
  -DBINDIR=\"$(exec_prefix)/bin\" \
  -DLOCALEDIR=\"${LOCALEDIR}\"\
  -DSYSCONFDIR=\"$(sysconfdir)/\" \
  -DPKGDATADIR=\"$(pkgdatadir)/\" \
  -DDATAROOTDIR=\"$(datarootdir)/\" \
  -DPKGNAME=\"historicHarpsichord\" \
  -DG_LOG_DOMAIN=\"HistoricHarpsichord\"

historicHarpsichord_LDADD = $(INTLLIBS) libaudiobackend.a -L$(top_builddir)/libs/libsffile -lsffile


pathconfig.h:  $(top_builddir)/config.status
	-@rm pathconfig.tmp 
	@echo "Generating pathconfig.h..."
	@echo '#define HISTORICHARPSICHORD_LOAD_PATH "@historicHarpsichord_load_path@"' >pathconfig.tmp
	@echo '#define HISTORICHARPSICHORD_BIN_PATH  "@historicHarpsichord_bin_path@"' >>pathconfig.tmp
	@mv pathconfig.tmp $@	



DISTCLEANFILES: pathconfig.h

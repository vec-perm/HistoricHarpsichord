@INTLTOOL_XML_RULE@
appdatadir = $(datarootdir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = historicHarpsichord.appdata.xml.in

SUBDIRS = \
  libs/libsffile \
  build \
  docs \
  pixmaps \
  po \
  src \
  soundfonts

EXTRA_DIST = \
  include \
  @PACKAGE@.spec.in \
  @PACKAGE@.spec \
  $(INTLTOOL_FILES) \
  docs/DESIGN \
  docs/GOALS \
  docs/DESIGN.lilypond \
  intltool-extract.in \
  intltool-merge.in \
  intltool-update.in \
  po/$(GETTEXT_PACKAGE).pot \
  historicHarpsichord.appdata.xml.in

MAINTAINERCLEANFILES = \
  Makefile.in \
  aclocal.m4 \
  configure \
  config.h.in \
  stamp-h.in \
  config.guess \
  mkinstalldirs \
  missing \
  install-sh \
  config.sub \
  acinclude.m4 \
  config.log \
  depcomp \
  intltool-extract \
  intltool-merge \
  intltool-update \
  po/.intltool-merge-cache \
  $(GETTEXT_PACKAGE)-$(VERSION).pot

DISTCLEANFILES = \
  intltool-extract \
  intltool-merge \
  intltool-update \
  po/.intltool-merge-cache \
  po/historicHarpsichord.pot

CLEANFILES = \
  $(appdata_DATA)

ACLOCAL_AMFLAGS = -I build

install-data-hook:
	

uninstall-hook:
	

potfile:
	@echo Generating POTFILES...
	@rm -f $(srcdir)/po/POTFILES.in
	@rm -f $(srcdir)/po/POTFILES.skip
	@cd $(srcdir) && find src -name '*.c' -o -name '*.h' ! -name 'pathconfig.h' | sort >> po/POTFILES.in
	@cd $(srcdir) && echo historicHarpsichord.appdata.xml.in >> po/POTFILES.in
	@cd $(srcdir) && find src -name 'pathconfig.h' | sort >> po/POTFILES.skip
	@cp $(srcdir)/po/POTFILES.in $(builddir)/po/POTFILES.in.bak
	@cp $(srcdir)/po/POTFILES.skip $(builddir)/po/POTFILES.skip.bak
	@mv $(builddir)/po/POTFILES.in.bak $(builddir)/po/POTFILES.in
	@mv $(builddir)/po/POTFILES.skip.bak $(builddir)/po/POTFILES.skip

pot:
	@echo Generating POT...
	$(MAKE) -C "$(builddir)/po" "$(GETTEXT_PACKAGE).pot"
#	cp "$(builddir)/po/$(GETTEXT_PACKAGE).pot" "$(builddir)/po/$(GETTEXT_PACKAGE)-$(VERSION).pot"

po/$(GETTEXT_PACKAGE).pot:pot

update-po:
	@echo Get po files from translationproject.org
	rsync -Lrtvz translationproject.org::tp/latest/historicHarpsichord/ po



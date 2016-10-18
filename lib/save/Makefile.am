## Makefile.am -- Process this file with automake to process Makefile.in

unangband_files = \
  delete.me

EXTRA_DIST = \
  $(unangband_files)

if SET_GID
unangbanddir = @DEFAULT_PATH@save

unangband_DATA = \
  $(unangband_files)
endif

install-data-hook:
if SET_GID
	chown -R root:@GAMEGROUP@ "$(DESTDIR)$(unangbanddir)"
	chmod -R 070 "$(DESTDIR)$(unangbanddir)"
endif

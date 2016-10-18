## Makefile.am -- Process this file with automake to process Makefile.in

unangband_files = \
  delete.me font-ibm.prf graf-gcu.prf graf-xxx.prf pref-win.prf \
  font-mac.prf graf-ibm.prf graf.prf pref-x11.prf \
  flvr-dvg.prf font-win.prf graf-iso.prf message.prf pref.prf \
  flvr-iso.prf font-x11.prf graf-lsl.prf pref-acn.prf user-mac.prf \
  flvr-new.prf font-xxx.prf graf-mac.prf pref-ami.prf user.prf \
  flvr-xxx.prf font.prf graf-new.prf pref-emx.prf xtra-dvg.prf \
  font-ami.prf graf-ami.prf graf-win.prf pref-gcu.prf xtra-new.prf \
  font-dos.prf graf-dvg.prf graf-x11.prf pref-mac.prf xtra-xxx.prf


EXTRA_DIST = \
  $(unangband_files)

if SET_GID
unangbanddir = @DEFAULT_PATH@pref

unangband_DATA = \
  $(unangband_files)
endif


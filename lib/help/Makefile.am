## Makefile.am -- Process this file with automake to process Makefile.in

unangband_files = \
  attack.txt bestiary.txt birth.hlp \
  birth.txt classes.txt cmddesc.txt cmdorig.txt \
  cmdrogue.txt command.hlp dungeon.hlp dungeon.txt \
  egoitem.txt faq.txt general.hlp \
  help.hlp macro.txt option.txt playing.txt \
  races.txt resist.txt rumours.hlp stats.txt styles.txt symbol.txt \
  usehelp.txt userpref.txt varattk.txt varcmds.txt\
  variant.hlp variant.txt varopt.txt version.txt welcome.txt

EXTRA_DIST = \
  $(unangband_files)

if SET_GID
unangbanddir = @DEFAULT_PATH@help

unangband_DATA = \
  $(unangband_files)
endif

## Makefile.am -- Process this file with automake to process Makefile.in

unangband_files = \
  artifact.txt \
  cost_adj.txt \
  dungeon.txt \
  ego_item.txt \
  flavor.txt \
  limits.txt \
  misc.txt \
  monster.txt \
  names.txt \
  object.txt \
  p_class.txt \
  p_hist.txt \
  p_race.txt \
  quest.txt \
  room.txt \
  rune.txt \
  shop_own.txt \
  spell.txt \
  store.txt \
  style.txt \
  terrain.txt \
  vault.txt \
  w_info.txt 

EXTRA_DIST = \
  $(unangband_files)

if SET_GID
unangbanddir = @DEFAULT_PATH@edit

unangband_DATA = \
  $(unangband_files)
endif


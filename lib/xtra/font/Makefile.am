## Makefile.am -- Process this file with automake to process Makefile.in

angband_files = \
  16x16.txt

bdf_files = \
  10x14x.bdf \
  10x14xb.bdf \
  12x18x.bdf \
  16x16x.bdf \
  5x8x.bdf \
  6x10x.bdf \
  8x12x.bdf \
  8x12xb.bdf \
  8x8x.bdf \
  8x8xb.bdf \
  8x16x.bdf \
  16x24x.bdf

pcf_gz_files := $(patsubst %.bdf,%.pcf.gz,$(bdf_files))

%.pcf.gz: %.pcf
	gzip -9 -f $<

%.pcf: %.bdf
	bdftopcf -o $@ $<

fonts.dir: $(pcf_gz_files)
	mkfontdir
	
EXTRA_DIST = \
  $(angband_files) $(bdf_files) compile_bdf_fonts.sh

if SET_GID
angbanddir = @DEFAULT_PATH@xtra/font

angband_DATA = \
  $(angband_files) $(pcf_gz_files) fonts.dir
endif

CLEANFILES = $(pcf_gz_files) fonts.dir

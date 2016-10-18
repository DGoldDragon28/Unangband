COMMONOBJS = \
	birth$(OBJEXT) \
	cave$(OBJEXT) \
	cmd1$(OBJEXT) \
	cmd2$(OBJEXT) \
	cmd3$(OBJEXT) \
	cmd4$(OBJEXT) \
	cmd5$(OBJEXT) \
	cmd6$(OBJEXT) \
	info$(OBJEXT) \
	dungeon$(OBJEXT) \
	files$(OBJEXT) \
	generate$(OBJEXT) \
	init1$(OBJEXT) \
	init2$(OBJEXT) \
	load$(OBJEXT) \
	melee1$(OBJEXT) \
	melee2$(OBJEXT) \
	monster1$(OBJEXT) \
	monster2$(OBJEXT) \
	object1$(OBJEXT) \
	object2$(OBJEXT) \
	option$(OBJEXT) \
	randart$(OBJEXT) \
	save$(OBJEXT) \
	spells1$(OBJEXT) \
	spells2$(OBJEXT) \
	store$(OBJEXT) \
	tables$(OBJEXT) \
	util$(OBJEXT) \
	variable$(OBJEXT) \
	wizard1$(OBJEXT) \
	wizard2$(OBJEXT) \
	xtra1$(OBJEXT) \
	xtra2$(OBJEXT) \
	borgdumb$(OBJEXT) \
	z-form$(OBJEXT) \
	z-rand$(OBJEXT) \
	z-term$(OBJEXT) \
	z-util$(OBJEXT) \
	z-virt$(OBJEXT) \

HDRS = \
	h-basic.h \
	h-define.h h-type.h \
	h-system.h h-config.h

INCS = \
	angband.h \
	config.h defines.h option.h \
	types.h externs.h osx/osx.h \
	z-term.h z-rand.h \
	z-util.h z-virt.h \
	z-form.h $(HDRS)

# Addditional dependencies

borgdumb$(OBJEXT): borgdumb.h

z-form$(OBJEXT): z-form.h z-util.h z-virt.h

z-rand$(OBJEXT): z-rand.h

z-term$(OBJEXT): z-term.h z-virt.h

z-util$(OBJEXT): z-util.h

z-virt$(OBJEXT): z-virt.h z-util.h


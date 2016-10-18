## Makefile.am -- Process this file with automake to process Makefile.in

unangband_files = \
  delete.me

EXTRA_DIST = \
  $(unangband_files)

if SET_GID
unangbanddir = @DEFAULT_PATH@data

unangband_DATA = \
  $(unangband_files)
endif


install-data-hook:
if SET_GID
	if test "@GAMEGROUP@" != ""; then \
	  touch "$(DESTDIR)$(unangbanddir)/artifact.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/cost_adj.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/dungeon.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/ego_item.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/flavor.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/limits.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/misc.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/monster.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/names.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/object.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/p_class.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/p_hist.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/p_race.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/quest.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/room.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/rune.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/shop_own.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/spell.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/store.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/style.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/terrain.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/vault.raw" && \
	  touch "$(DESTDIR)$(unangbanddir)/w_info.raw" && \
	  chown -R root:@GAMEGROUP@ "$(DESTDIR)$(unangbanddir)" && \
	  chmod -R g+w "$(DESTDIR)$(unangbanddir)" ;\
	fi
endif


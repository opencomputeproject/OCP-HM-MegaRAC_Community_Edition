GDBUS_APPS = bmcctl \
	     flashbios \
	     op-flasher \
	     op-hostctl \
	     op-pwrctl \
	     pwrbutton \
	     rstbutton

SUBDIRS = fanctl \
	  ledctl \
	  libopenbmc_intf \
	  pychassisctl \
	  pydownloadmgr \
	  pyflashbmc \
	  pyinventorymgr \
	  pyipmitest \
	  pystatemgr \
	  pysystemmgr \
	  pytools

REVERSE_SUBDIRS = $(shell echo $(SUBDIRS) $(GDBUS_APPS) | tr ' ' '\n' | tac |tr '\n' ' ')

.PHONY: subdirs $(SUBDIRS) $(GDBUS_APPS)

subdirs: $(SUBDIRS) $(GDBUS_APPS)

$(SUBDIRS):
	$(MAKE) -C $@

$(GDBUS_APPS): libopenbmc_intf
	$(MAKE) -C $@ CFLAGS="-I ../$^" LDFLAGS="-L ../$^"

install: subdirs
	@for d in $(SUBDIRS) $(GDBUS_APPS); do \
		$(MAKE) -C $$d $@ DESTDIR=$(DESTDIR) PREFIX=$(PREFIX) || exit 1; \
	done
clean:
	@for d in $(REVERSE_SUBDIRS); do \
		$(MAKE) -C $$d $@ || exit 1; \
	done

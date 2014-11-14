TOPDIR	:= $(shell /bin/pwd)

export TOPDIR

.PHONY: util FslExt EXA all clean install

all : FslExt util EXA

FslExt:
	@echo
	@echo Invoking FslExt make...
	$(MAKE) -C $(TOPDIR)/FslExt/src -f makefile.linux

util:
	@echo
	@echo Invoking util make...
	$(MAKE) -C $(TOPDIR)/util/autohdmi -f makefile.linux

EXA:
	@echo
	@echo Invoking EXA make...
	$(MAKE) -C $(TOPDIR)/EXA/src -f makefile.linux

install:
	mkdir -p $(prefix)/bin
	cp $(TOPDIR)/util/autohdmi/autohdmi $(prefix)/bin/
	chmod 700 $(prefix)/bin/autohdmi
	mkdir -p $(prefix)/lib/
	cp $(TOPDIR)/FslExt/src/libfsl_x11_ext.so $(prefix)/lib/
	mkdir -p $(prefix)/lib/xorg/modules/drivers/
	cp $(TOPDIR)/EXA/src/vivante_drv.so $(prefix)/lib/xorg/modules/drivers/

clean:
	$(MAKE) -C $(TOPDIR)/util/autohdmi -f makefile.linux $@
	$(MAKE) -C $(TOPDIR)/FslExt/src -f makefile.linux $@
	$(MAKE) -C $(TOPDIR)/EXA/src -f makefile.linux $@

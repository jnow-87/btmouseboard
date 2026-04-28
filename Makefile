####
## init
####

# init build system
project_type := c
scripts_dir := scripts/build

# init config system
use_config_sys := y
config_ftype := Pconfig
config := .config
config_tree := scripts/config

# init code coverage system
use_coverage_sys := n

# include config
-include $(config)

# external dependencies
ifeq ($(CONFIG_BT_BUILD_FIRMWARE),y)
tool_deps := arduino-cli
endif

# init source and build tree
default_build_tree := build/$(CONFIG_BUILD_TYPE)/
src_dirs := controller

# include build system Makefile
include $(scripts_dir)/main.make

# init default flags
cflags := \
	$(CFLAGS) \
	--std=c23

cppflags := \
	$(CPPFLAGS) \
	-I$(build_tree) \
	-Iinclude

ldflags := $(LDFLAGS)
ldlibs := $(LDLIOBSFLAGS)

####
## targets
####

.PHONY: all
ifeq ($(CONFIG_BUILD_DEBUG),y)
all: cflags += -g
all: cxxflags += -g
all: asflags += -g
endif

all: $(lib) $(bin)

.PHONY: clean
clean:
	$(rm) $(filter-out $(build_tree)/$(scripts_dir),$(wildcard $(build_tree)/*))

.PHONY: distclean
distclean:
	$(rm) $(config) $(build_tree)

## install
include $(scripts_dir)/install.make
symlink := $(QUTIL)ln -srf

.PHONY: install
install: all
	$(call install,$(build_tree)/controller/mb)
	$(symlink) $(PREFIX)/mb $(PREFIX)/btmb
	$(symlink) $(PREFIX)/mb $(PREFIX)/btmbmac
	$(symlink) $(PREFIX)/mb $(PREFIX)/xmb

.PHONY: install-system
install-system: all
	$(call install,$(build_tree)/backend/x11/xmbrecv/xmbrecv,/usr/bin)
	$(call install,system/xmbrecv.service,/etc/systemd/system/)
	systemctl enable xmbrecv.service

.PHONY: uninstall
uninstall:
	$(call uninstall,$(PREFIX)/mb)
	$(call uninstall,$(PREFIX)/btmb)
	$(call uninstall,$(PREFIX)/btmbmac)
	$(call uninstall,$(PREFIX)/xmb)

.PHONY: uninstall-system
uninstall-system:
	systemctl disable xmbrecv.service
	$(call uninstall,/etc/systemd/system/xmbrecv.service)
	$(call uninstall,/usr/bin/xmbrecv)

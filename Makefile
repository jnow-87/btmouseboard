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

# external dependencies
tool_deps := arduino-cli

# include config
-include $(config)

# init source and build tree
default_build_tree := build/$(CONFIG_BUILD_TYPE)/
src_dirs := controller firmware

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

.PHONY: install
install: all
	$(call install,$(build_tree)/controller/btmouseboard)

.PHONY: uninstall
uninstall:
	$(call uninstall,$(PREFIX)/btmouseboard)

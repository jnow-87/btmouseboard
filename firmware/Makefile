# arduino-cli is run from within loc_src_tree instead of the project root, hence the
# build system location variables, such as $(build_tree) are not valid anymore and
# the actual build directory has to be captured separately here
build_dir := "../$(loc_build_tree)"

compile_flags := \
	-I../include \
	-I../$(build_tree)/config

libs := \
	NimBLE-Arduino


# run arduino-cli
#
#	$(call arduino_cli,<cli-args>)
define arduino_cli
	$(call cmd_run_script, \
		cd firmware; \
		arduino-cli \
			--config-file "arduino-cli.yaml" \
			$1
	)
endef

# run arduino-cli compile
#
#	$(call arduino_compile,<compile-cli-args>)
define arduino_compile
	$(call arduino_cli, \
		compile \
		-b "$(CONFIG_ARDUINO_PACKAGE):$(CONFIG_ARDUINO_ARCH):$(CONFIG_ARDUINO_BOARD)" \
		--build-path $(build_dir) \
		--build-property compiler.cpp.extra_flags="$(compile_flags)" \
		$(patsubst %,--library %,$(libs)) \
		$1
	)
endef


.PHONY: arduino-sys-files-prepare
arduino-sys-files-prepare:
	$(call arduino_cli, core install "$(CONFIG_ARDUINO_PACKAGE):$(CONFIG_ARDUINO_ARCH)")

.PHONY: firmware
firmware:
	$(call arduino_compile, --verify)

all: firmware

.PHONY: flash-firmware
flash-firmware:
	$(call arduino_compile, --upload --port $(CONFIG_ARDUINO_PROG_PORT))

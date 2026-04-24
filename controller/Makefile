bin-y := mb


mb-y := \
	events.o \
	log.o \
	main.o \
	opts.o \
	render.o \
	backend/ \
	shared/


mb-cppflags := \
    -I/usr/include/freetype2 \
    -D_DEFAULT_SOURCE

mb-ldlibs := \
    -lX11 \
    -lfontconfig \
    -lXft \
	-lXfixes

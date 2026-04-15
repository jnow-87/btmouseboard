bin-y := btmouseboard


btmouseboard-y := \
	events.o \
	log.o \
	main.o \
	opts.o \
	render.o \
	backend/ \
	shared/


btmouseboard-cppflags := \
    -I/usr/include/freetype2 \
    -D_DEFAULT_SOURCE

btmouseboard-ldlibs := \
    -lX11 \
    -lfontconfig \
    -lXft \
	-lXfixes

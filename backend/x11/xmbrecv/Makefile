bin-y := xmbrecv


xmbrecv-y := \
	main.o \
	log.o \
	opts.o \
	shared/

xmbrecv-cppflags := \
	-I/usr/include/freetype2 \
	-D_DEFAULT_SOURCE

xmbrecv-ldlibs := \
	-lX11 \
	-lXfixes \
	-lXft \
	-lXtst \
	-lfontconfig

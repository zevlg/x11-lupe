SRCS = lupe.c option.c timer.c avionics.c
OBJS = lupe.o option.o timer.o avionics.o
SYS_LIBRARIES = -lX11 -lXext -lm

HEADERS = config.h extern.h version.h timer.h

lupe: $(OBJS)
	gcc -o lupe $(OBJS) $(SYS_LIBRARIES)

$(OBJS): $(HEADERS)

config.h: configure config.h.in
	/bin/sh configure

lupe.o: icon.xbm

clean:
	rm $(OBJS) lupe config.h

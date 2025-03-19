CFILES := wakit.c dynamic_string.c x11.c cli_io.c rofi.c
OFILES = $(CFILES:.c=.o)

CC := gcc
LDFLAGS := -lX11
# CFLAGS := -g

all: wakit

wakit: $(OFILES)
	$(CC) $(LDFLAGS) $(OFILES) -o wakit

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

run: wakit
	./wakit

clean:
	rm wakit $(OFILES)

make: wakit.c dynamic_string.c x11.c cli_io.c rofi.c
	cc wakit.c dynamic_string.c x11.c cli_io.c rofi.c -o wakit -lX11 -g

run: wakit
	./wakit

make: wakit.c dynamic_string.c x11.c cli_io.c
	gcc cli_io.c x11.c dynamic_string.c wakit.c -o wakit -lX11 -g
	./wakit

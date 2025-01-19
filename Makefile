make: wakit.c dynamic_string.c x11.c
	gcc x11.c dynamic_string.c wakit.c -o wakit
	./wakit

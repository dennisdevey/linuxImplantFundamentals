# Vicente Tovar- Sniffex Makefile
FILE ?= sniffex
INTERFACE ?= lo
sniffex: sniffex.c
	gcc -o sniffex sniffex.c -lpcap

sniffex_debug: sniffex.c
	gcc -DDEBUG -o sniffex_debug sniffex.c -lpcap

run_lo:
	sudo ./$(FILE) $(INTERFACE)

valgrind:
	sudo valgrind --track-origins=yes --leak-check=full ./$(FILE) $(INTERFACE)
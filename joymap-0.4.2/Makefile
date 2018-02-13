# Disable -Werror if there are compile errors 
CFLAGS+=-Wall -Werror -g
all: loadmap reserve_js

loadmap: loadmap.o dictionary.o mapparser.o programparser.o validkeys.o events.o vm.o devices.o config.o
	$(CC) -g -o loadmap loadmap.o dictionary.o mapparser.o programparser.o validkeys.o events.o vm.o devices.o config.o

reserve_js: reserve_js.o config.o
	$(CC) -g -o reserve_js reserve_js.o config.o

clean:
	rm -f reserve_js reserve_js.o loadmap loadmap.o dictionary.o mapparser.o programparser.o validkeys.o events.o vm.o devices.o config.o

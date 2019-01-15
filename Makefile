all: display-daemon

display-daemon: display-daemon.c font5x7.c font5x7.h
	arm-linux-gnueabihf-gcc display-daemon.c font5x7.c -g -Wall -o display-daemon

install: display-daemon
	cp $< $(DESTDIR)$(PREFIX)/display-daemon

clean:
	rm display-daemon

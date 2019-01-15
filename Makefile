all: display

display: display.c font5x7.c font5x7.h
	arm-linux-gnueabihf-gcc display.c font5x7.c -g -Wall -o display

install: display
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $< $(DESTDIR)$(PREFIX)/bin/display

clean:
	rm display

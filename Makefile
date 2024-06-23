SOURCES=submodules/fuzzy-match/fuzzy_match.c main.c

tty-cheatsheet: $(SOURCES)
	$(CC) -o $@ $(CFLAGS) $(SOURCES) -lncurses

clean:
	rm tty-cheatsheet

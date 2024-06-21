SOURCES=submodules/fuzzy-match/fuzzy_match.c main.c

tty-cheatsheet: $(SOURCES)
	$(CC) -o $@ $(CFLAGS) $(SOURCES)

clean:
	rm tty-cheatsheet

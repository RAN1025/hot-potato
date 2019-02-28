CFLAGS=-Wall -Werror -std=gnu99 -pedantic -ggdb3
TARGETS=ringmaster player

all: $(TARGETS)
clean:
	rm -f $(TARGETS) *~

ringmaster: ringmaster.c
	gcc $(CFLAGS) -o $@ $<

player: player.c
	gcc $(CFLAGS) -o $@ $<

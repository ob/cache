cache: cache.c Makefile
	$(CC) -o $@ -Wall -O0 -g $<

cache.s: cache.c Makefile
	$(CC) -S $<


cache: cache.c Makefile
	$(CC) -o $@ -O0 -g $<

cache.s: cache.c Makefile
	$(CC) -S $<


PYTHON  = uv run python
ANALYZE = $(PYTHON) analyze.py

# Override per invocation: make run BUFFER=67108864 GHZ=2.6
HOSTNAME = $(shell hostname -s | cut -d. -f1)
STAMP    = $(shell date +%Y%m%d-%H%M%S)

cache: cache.c Makefile
	$(CC) -o $@ -Wall -O0 -g $<

cache.s: cache.c Makefile
	$(CC) -S $<

# Run a benchmark, then analyze and plot the result.
#   make run                      # 1 GB random walk (default; ~minutes)
#   make run BUFFER=67108864      # 64 MB only (faster)
#   make run GHZ=2.6              # add cycle column to the report
.PHONY: run
run: cache
	@mkdir -p results images
	@OUTFILE=results/$(HOSTNAME)-$(STAMP); \
	echo "==> ./cache -o $$OUTFILE"; \
	./cache -o $$OUTFILE $(if $(BUFFER),--buffer $(BUFFER)); \
	echo; \
	echo "==> report"; \
	$(ANALYZE) $$OUTFILE $(if $(GHZ),--ghz $(GHZ)); \
	echo; \
	echo "==> plots"; \
	$(ANALYZE) $$OUTFILE --plot 3d -o images/$$(basename $$OUTFILE).3d.png; \
	$(ANALYZE) $$OUTFILE --plot 2d -o images/$$(basename $$OUTFILE).2d.png

.PHONY: clean
clean:
	rm -f cache cache.s
	rm -rf cache.dSYM

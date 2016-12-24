
LDFLAGS= -lm 
CFLAGS= -O3 

.PHONY: all
all: lisper

lisper: main.c lisper.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) 

.PHONY: clean
clean:
	rm lisper

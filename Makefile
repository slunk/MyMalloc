CC=clang
CSOURCES=my_malloc.c fat_malloc.c thin_malloc.c tree_malloc.c
CFLAGS=-DUSE_TREE_MALLOC
OBJECTS=$(CSOURCES:.c=.o)

all: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o my_malloc

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o my_malloc

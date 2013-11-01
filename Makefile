CSOURCES=my_malloc.c fat_malloc.c thin_malloc.c
OBJECTS=$(CSOURCES:.c=.o)

all: $(OBJECTS)
	gcc $(OBJECTS) -o my_malloc

.c.o:
	gcc -c $< -o $@

clean:
	rm -f *.o my_malloc

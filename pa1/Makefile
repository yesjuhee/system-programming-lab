TARGET=pa1
CFLAGS=-Wall
OBJECTS=pa1.o my_string.o my_stdio.o case1.o case2.o case3.o case4.o search.o

.PHONY: clean

$(TARGET): $(OBJECTS)
	gcc $(CFLAGS) -o $@ $^
pa1.o: pa1.c
	gcc $(CFLAGS) -c $<
my_string.o: my_string.c
	gcc $(CFLAGS) -c $<
my_stdio.o: my_stdio.c
	gcc $(CFLAGS) -c $<
case1.o: case1.c
	gcc $(CFLAGS) -c $<
case2.o: case2.c
	gcc $(CFLAGS) -c $<
case3.o: case3.c
	gcc $(CFLAGS) -c $<
case4.o: case4.c
	gcc $(CFLAGS) -c $<
search.o: search.c
	gcc $(CFLAGS) -c $<
clean:
	rm -f $(OBJECTS) $(TARGET)
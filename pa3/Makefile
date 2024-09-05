CC=gcc
CFLAGS=-Wall -D_GNU_SOURCE 

all: pa3_server pa3_client

pa3_server: server.c
	$(CC) $(CFLAGS) -o $@ $^ -largon2

pa3_client: client.c
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

clean:
	rm -rf *.o pa3_server pa3_client

.PHONY: all clean
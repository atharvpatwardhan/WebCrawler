CC = gcc
CFLAGS = -std=c11 -pedantic -pthread -Wall -Werror -fsanitize=address,undefined -lm -g -lcurl

all: crawler

crawler: crawler.c
	$(CC) $(CFLAGS) crawler.c -o crawler.o

clean:
	rm -f crawler.o *~

run: crawler
	./crawler.o https://sitecorediaries.org/tag/dummy-website/

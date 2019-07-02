SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2

all: clean main

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

main: $(OBJS)
	gcc $(OBJS) -o main

clean:
	rm main *.o >/dev/null 2>/dev/null || true
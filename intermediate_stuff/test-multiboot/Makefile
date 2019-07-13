SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
CFLAGS = -Wall -O2 -ffreestanding -nostdinc -nostdlib -nostartfiles

all: clean kernel8.img

test.o: test.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c test.S -o test.o

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

kernel8.img: test.o $(OBJS)
	aarch64-linux-gnu-ld -nostdlib -nostartfiles test.o $(OBJS) -T linker.ld -o kernel8.elf
#	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
	mv kernel8.elf kernel8.img

clean:
	rm kernel8.elf *.o >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial stdio
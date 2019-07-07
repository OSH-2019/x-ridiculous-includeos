SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
ELF_BIN = $(wildcard *.elf.bin)
ELO = $(ELF_BIN:.elf.bin=.elo)
DTB = $(wildcard *.dtb)
DTO = $(DTB:.dtb=.dto)
CFLAGS = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -g

all: clean kernel8.img

test.o: test.S
	aarch64-linux-gnu-gcc $(CFLAGS) -c test.S -o test.o

%.o: %.c
	aarch64-linux-gnu-gcc $(CFLAGS) -c $< -o $@

# !Make sure there is no .elf output
kernel8.img: test.o $(OBJS) $(DTO) $(ELO)
	aarch64-linux-gnu-ld -nostdlib -nostartfiles test.o $(OBJS) $(DTO) $(ELO) -T linker.ld -o kernel8.elf
	aarch64-linux-gnu-objcopy -O binary kernel8.elf kernel8.img
#	mv kernel8.elf kernel8.img

# DTB Objects
%.dto: %.dtb
	aarch64-linux-gnu-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

# Target Multiboot ELF binary to be added
%.elo: %.elf.bin
	aarch64-linux-gnu-objcopy -I binary -O elf64-littleaarch64 -B aarch64 $< $@

clean:
	rm kernel8.elf *.o *.dto *.elo >/dev/null 2>/dev/null || true

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial stdio -drive file=sd.img,format=raw,if=sd

debug:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -S -gdb tcp::2345 -drive file=sd.img,format=raw,if=sd
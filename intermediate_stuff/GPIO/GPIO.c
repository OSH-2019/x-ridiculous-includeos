#include "GPIO.h"

__uint32_t read_peri(volatile __uint32_t *paddr)
{
#ifdef GPIO_DEBUG
    printf("read paddr:%p\n", (void *)paddr);
#endif
    __uint32_t data;
    //__sync_synchronize();
    data = *paddr;
    //__sync_synchronize();
    return data;
}

void write_peri(volatile __uint32_t *paddr, __uint32_t value)
{
#ifdef GPIO_DEBUG
    printf("write paddr:%p write data:%08X\n", paddr, value);
#endif
    //__sync_synchronize();     use it if necessary.
    *paddr = value;
    //__sync_synchronize();     use it if necessary.
}

void gpio_set_bits(volatile __uint32_t *paddr, __uint32_t mask, __uint32_t value)
{
    __uint32_t ori = read_peri(paddr);
    __uint32_t val = (ori & ~mask) | (value & mask);
    write_peri(paddr, val);
}

/* 
pin is a GPIO pin number NOT RPi pin number
There are 6 control registers, each control the functions of a block
of 10 pins.
Each control register has 10 sets of 3 bits per GPIO pin:
000 = GPIO Pin X is an input
001 = GPIO Pin X is an output
100 = GPIO Pin X takes alternate function 0
101 = GPIO Pin X takes alternate function 1
110 = GPIO Pin X takes alternate function 2
111 = GPIO Pin X takes alternate function 3
011 = GPIO Pin X takes alternate function 4
010 = GPIO Pin X takes alternate function 5

ALT 0: Where most of the interesting and useful alternate functions are as far as 
the Raspberry Pi is concerned. The SDA and SCL 0 and 1 are the two I2C buses, 
and the TXD0 and RXD0 are the serial connections. The GPCLK lines are 
a general-purpose clock output that can be set to run at a fixed frequency 
independent of any software. The PWM pins provide the two pulse width modulated outputs; 
the SPI 0 is the serial peripheral interface bus lines. Finally, 
the PCM pins provide pulse code modulated audio outputs.

ALT 1: The pins are used as a secondary memory bus. Due to the design of the Raspberry Pi, 
this is of no use at all.

ALT 2: The only ALT 2 pins brought out the to the GPIO pin header are reserved.

ALT 3: The most useful pins here are the CTS0 and RTS0 lines; these are handshaking lines 
for the serial module if you need them. The BSC lines are for the Broadcom Serial Controller, 
which is a fast mode I2C-compliant bus supporting 7-bit and 10-bit addressing and having 
the timing controlled by internal registers. The SD1 lines are probably for the control of 
an SD card, but the BCM2835 ARM Peripherals document makes no other mention of it. 
It isn’t the way the Raspberry Pi accesses the SD card anyway.

ALT 4: The SPI 1 lines are a second SPI bus. And the ARM pins are for a JTAG interface. 
JTAG is a way of talking to the chip without any software on it. 
It’s very much used for the initial tests on a system during development, 
although it can be used for hardware debugging as well.

ALT 5: The useful pins here are the second serial port data and handshaking lines. 
The PWM lines are exactly the same PWM lines that are switches to GPIO 12 and 13 under ALT 0, 
only this time they’re switched to GPIO 20 and 21. There are also two of the general-purpose 
clock lines along with another copy of the ARM JTAG signals.
*/

void gpio_func_select(const __uint8_t pin, __uint8_t mode)
{
    volatile __uint32_t *paddr = GPIO_BASE + GPFSEL0 / 4 + (pin / 10);
    __uint8_t shift = (pin % 10) * 3;
    __uint32_t mask = GPIO_FSEL_MASK << shift;
    __uint32_t value = mode << shift;
    gpio_set_bits(paddr, mask, value);
}

void gpio_set(const __uint8_t pin)
{
    volatile __uint32_t *paddr = GPIO_BASE + GPSET0 / 4 + pin / 32;
    __uint8_t shift = (pin % 32);
    write_peri(paddr, 1 << shift);
}

void gpio_clr(const __uint8_t pin)
{
    volatile __uint32_t *paddr = GPIO_BASE + GPCLR0 / 4 + pin / 32;
    __uint8_t shift = pin % 32;
    write_peri(paddr, 1 << shift);
}

int gpio_read_level(const __uint8_t pin)
{
    volatile __uint32_t *paddr = GPIO_BASE + GPLEV0 / 4 + pin / 32;
    __uint8_t shift = pin % 32;
    __uint32_t value = read_peri(paddr);
    return (value & (1 << shift)) ? HIGH : LOW;
}

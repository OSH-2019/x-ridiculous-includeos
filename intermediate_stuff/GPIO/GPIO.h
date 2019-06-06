//#include <stdio.h>


#define GPIO_BASE 0x3F000000

#define GPFSEL0 ((volatile unsigned int)(GPIO_BASE + 0x00200000))
#define GPFSEL1 ((volatile unsigned int)(GPIO_BASE + 0x00200004))
#define GPFSEL2 ((volatile unsigned int)(GPIO_BASE + 0x00200008))
#define GPFSEL3 ((volatile unsigned int)(GPIO_BASE + 0x0020000C))
#define GPFSEL4 ((volatile unsigned int)(GPIO_BASE + 0x00200010))
#define GPFSEL5 ((volatile unsigned int)(GPIO_BASE + 0x00200014))
#define GPSET0 ((volatile unsigned int)(GPIO_BASE + 0x0020001C))
#define GPSET1 ((volatile unsigned int)(GPIO_BASE + 0x00200020))
#define GPCLR0 ((volatile unsigned int)(GPIO_BASE + 0x00200028))
#define GPLEV0 ((volatile unsigned int)(GPIO_BASE + 0x00200034))
#define GPLEV1 ((volatile unsigned int)(GPIO_BASE + 0x00200038))
#define GPEDS0 ((volatile unsigned int)(GPIO_BASE + 0x00200040))
#define GPEDS1 ((volatile unsigned int)(GPIO_BASE + 0x00200044))
#define GPHEN0 ((volatile unsigned int)(GPIO_BASE + 0x00200064))
#define GPHEN1 ((volatile unsigned int)(GPIO_BASE + 0x00200068))
#define GPPUD ((volatile unsigned int)(GPIO_BASE + 0x00200094))
#define GPPUDCLK0 ((volatile unsigned int)(GPIO_BASE + 0x00200098))
#define GPPUDCLK1 ((volatile unsigned int)(GPIO_BASE + 0x0020009C))

#define HIGH 1
#define LOW 0

#define GPIO_FSEL_INPT 0x00  /*!< Input 0b000 */
#define GPIO_FSEL_OUTP 0x01  /*!< Output 0b001 */
#define GPIO_FSEL_ALT0 0x04  /*!< Alternate function 0 0b100 */
#define GPIO_FSEL_ALT1 0x05  /*!< Alternate function 1 0b101 */
#define GPIO_FSEL_ALT2 0x06  /*!< Alternate function 2 0b110, */
#define GPIO_FSEL_ALT3 0x07  /*!< Alternate function 3 0b111 */
#define GPIO_FSEL_ALT4 0x03  /*!< Alternate function 4 0b011 */
#define GPIO_FSEL_ALT5 0x02  /*!< Alternate function 5 0b010 */
#define GPIO_FSEL_MASK 0x07  /*!< Function select bits mask 0b111 */

typedef unsigned int __uint32_t;
typedef unsigned char __uint8_t;

/* Read 32 bit value from a peripheral address. */
__uint32_t read_peri(volatile __uint32_t *paddr);

/*  Write 32 bit value to a peripheral address. */ 
void write_peri(volatile __uint32_t *paddr, __uint32_t value);

/* Set bits in the address. */
void gpio_set_bits(volatile __uint32_t *paddr, __uint32_t mask, __uint32_t value);

/* Select the function of GPIO. */
void gpio_func_select(const __uint8_t pin, __uint8_t mode);

/* Set output pin. */
void gpio_set(const __uint8_t pin);

/* Clear output pin. */
void gpio_clr(const __uint8_t pin);

/* Read the current level of input pin. */
int gpio_read_level(const __uint8_t pin);
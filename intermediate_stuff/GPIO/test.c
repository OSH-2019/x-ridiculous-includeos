#include "GPIO.h"

int main()
{
    volatile __uint8_t led = 29;
    volatile __uint32_t *timer_addr = 0x3F003000;

    gpio_func_select(led, GPIO_FSEL_OUTP);
    int i;
    __uint32_t prev_time = read_peri(timer_addr);
    for (i = 0; i < 200; i++)
    {
        gpio_set(led);
        for (;;)
        {
            __uint32_t curr_time = read_peri(timer_addr);
            if (curr_time - prev_time >= 0x000f4240)
            {
                prev_time = curr_time;
                break;
            }
        }
        gpio_clr(led);
        for (;;)
        {
            __uint32_t curr_time = read_peri(timer_addr);
            if (curr_time - prev_time >= 0x000f4240)
            {
                prev_time = curr_time;
                break;
            }
        }
    }
    return 0;
}
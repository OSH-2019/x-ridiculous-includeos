#include <mbox.h>
#include <uart.h>
#include <GPIO.h>

/* Initialize framebuffer. */
int screen_init();

/* Print string on the screen */
void screen_print(const char *s);
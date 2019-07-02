#include "uart.h"

void _putchar(char ch) {
  uart_send(ch);
}
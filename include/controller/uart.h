#ifndef uart_H
#define uart_H


#include <stdbool.h>
#include <stdint.h>
#include <protocol.h>


/* types */
typedef struct{
	int fd;
	unsigned int dev_num;
	bool connected;
} uart_t;


/* prototypes */
uart_t *uart_init(void);
void uart_destroy(uart_t *uart);

int uart_stop(uart_t *uart);

int uart_key(uart_t *uart, uint8_t key, bool press);
int uart_button(uart_t *uart, uint8_t button, bool press);
int uart_move(uart_t *uart, int8_t dx, int8_t dy);


#endif // uart_H

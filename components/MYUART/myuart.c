#include "myuart.h"
#include "driver/uart.h"
#include "driver/gpio.h"

void uart_init(void)
{
    uart_config_t uart_structure = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .parity = UART_PARITY_DISABLE,
        .rx_flow_ctrl_thresh = 100,
        .source_clk = UART_SCLK_DEFAULT,
        .stop_bits = UART_STOP_BITS_1,
    };
    uart_param_config(UART_NUM_1, &uart_structure);

    uart_set_pin(UART_NUM_1, GPIO_NUM_17, GPIO_NUM_18, -1, -1);

    uart_driver_install(UART_NUM_1, 1024, 1024, 0, NULL,0);
}
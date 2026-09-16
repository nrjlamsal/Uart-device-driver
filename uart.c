#include <uart.h>

static void uart2_config_tx_pin(void) {
    int gpio_pin = 17;

   // write 2 on the io mux register
    uint32_t io_mux_val = REG_READ(IO_MUX_GPIO17_REG);
    io_mux_val = (io_mux_val & ~MCU_SEL_MASK) | MCU_SEL_GPIO;
    REG_WRITE(IO_MUX_GPIO17_REG, io_mux_val);

    // 2. connect tx index(peripheral) to the pin 17
    
    uint32_t out_sel_addr = GPIO_FUNC_OUT_SEL_BASE + (gpio_pin * 4);
    REG_WRITE(out_sel_addr, U2TXD_OUT_IDX);  // 4

    // 3. Enable output driver (master power switch for output)
    //    GPIO_ENABLE_REG bit 17 = 1
    REG_SET_BIT(GPIO_ENABLE_REG, (1 << gpio_pin));
}

static void uart2_config_rx_pin(void) {
    int gpio_pin = 16;

    // 1. IO_MUX: Set MCU_SEL (bits 14:12) to 2 (GPIO function)
    //    Clear bits 14:12 first, then set to 2
    uint32_t io_mux_val = REG_READ(IO_MUX_GPIO16_REG);
    io_mux_val = (io_mux_val & ~MCU_SEL_MASK) | MCU_SEL_GPIO;
    
    //    Enable input buffer (FUN_IE = 1)
    io_mux_val |= FUN_IE;
    REG_WRITE(IO_MUX_GPIO16_REG, io_mux_val);

    // 2. GPIO Matrix: Route GPIO16 to UART2 RX signal (index 4)
    //    Write pin number 16 to GPIO_FUNC4_IN_SEL_CFG_REG
    uint32_t in_sel_addr = GPIO_FUNC_IN_SEL_BASE + (U2RXD_IN_IDX * 4);
    REG_WRITE(in_sel_addr, gpio_pin);          // Write pin number 16
    REG_SET_BIT(in_sel_addr, (1 << 7));        // GPIO_SIG4_IN_SEL = 1 (enable matrix)

      // 3. Disable output driver (make it an input)
    //    GPIO_ENABLE_REG bit 16 = 0
    REG_CLR_BIT(GPIO_ENABLE_REG, (1 << gpio_pin));
}
#include "uart.h"
#include "stddef.h"
// #include "stdio.h" for NULL


static void uart2_config_tx_pin(void) {
    int gpio_pin = 17;

   // write 2 on the io mux register
    uint32_t io_mux_val = REG_READ(IO_MUX_GPIO17_REG);
    io_mux_val = (io_mux_val & ~MCU_SEL_MASK) | MCU_SEL_GPIO;
    REG_WRITE(IO_MUX_GPIO17_REG, io_mux_val);

    // 2. connect tx index(peripheral) to the pin 17
    
    uint32_t out_sel_addr = GPIO_FUNC_OUT_SEL_BASE + (gpio_pin * 4);
    REG_WRITE(out_sel_addr, U2TXD_OUT_IDX);  // 198

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

    // 2. GPIO Matrix: Route GPIO16 to UART2 RX signal (index 198)
    //    Write pin number 16 to GPIO_FUNC4_IN_SEL_CFG_REG
    uint32_t in_sel_addr = GPIO_FUNC_IN_SEL_BASE + (U2RXD_IN_IDX * 4);
    REG_WRITE(in_sel_addr, gpio_pin);          // Write pin number 16
    REG_SET_BIT(in_sel_addr, (1 << 7));        // GPIO_SIG4_IN_SEL = 1 (enable matrix)

      // 3. Disable output driver (make it an input)
    //    GPIO_ENABLE_REG bit 16 = 0
    REG_CLR_BIT(GPIO_ENABLE_REG, (1 << gpio_pin));
}

void uart2_flush_rx(void) {
    while (REG_READ(UART2_STATUS_REG) & UART_RXFIFO_CNT_MASK)
        (void)REG_READ(UART2_FIFO_REG);   // each read pops one byte
}

void uart2_init(const uart2_config_t *config) {

     // Step 1: Assert reset — wipes all internal flip-flops to 0
     REG_SET_BIT(DPORT_PERIP_RST_EN_REG, DPORT_UART2_RST_EN);

     // Step 2: Enable clocks — APB clock now feeds the (reset) peripheral
     REG_SET_BIT(DPORT_PERIP_CLK_EN_REG, DPORT_UART_MEM_CLK_EN); // shared FIFO memory clock
     REG_SET_BIT(DPORT_PERIP_CLK_EN_REG, DPORT_UART2_CLK_EN);     // UART2 module clock

     // Step 3: Release reset — peripheral starts cleanly from known zero state
     REG_CLR_BIT(DPORT_PERIP_RST_EN_REG, DPORT_UART2_RST_EN);

     uart2_config_tx_pin();
     uart2_config_rx_pin();

      uint32_t clk_div = 80000000 / config->baud_rate;
      REG_WRITE(UART2_CLKDIV_REG, clk_div);

      uint32_t conf0 = 0;
      conf0 |= UART_TICK_REF_ALWAYS_ON;

      switch (config->data_bits) {
        case 5:  conf0 |= UART_BIT_NUM_5; break;
        case 6:  conf0 |= UART_BIT_NUM_6; break;
        case 7:  conf0 |= UART_BIT_NUM_7; break;
        case 8:
        default: conf0 |= UART_BIT_NUM_8; break;
    }

    if (config->stop_bits == 2) {
        conf0 |= UART_STOP_BIT_NUM_2;
    } else {
        conf0 |= UART_STOP_BIT_NUM_1;  
    }

    if (config->parity == 1) {
        conf0 |= UART_PARITY_EN;        // Enable parity
        conf0 |= UART_PARITY;          // Odd parity
    } else if (config->parity == 2) {
        conf0 |= UART_PARITY_EN;        // Enable parit and Even parity menas it stays 0
    } else {
        conf0 &= ~UART_PARITY_EN;       // diable parity bit
    }

        REG_WRITE(UART2_CONF0_REG, conf0);


        uint32_t conf1 = 0;
        conf1 |= (1 << 31); // enable timeout interrupt
        conf1 |= (10 << 24); // threshold for timeout interrupt
        conf1 |= (32 << 0 );//  threshold for rxfifo interrupt

        REG_WRITE(UART2_CONF1_REG, conf1);

        uart2_flush_rx();

        REG_WRITE(UART2_INT_CLR_REG, 0xFFFFFFFF);
        REG_WRITE(UART2_INT_ENA_REG,UART_RXFIFO_FULL_INT|UART_FRM_ERR_INT |UART_RXFIFO_OVF_INT |UART_RXFIFO_TOUT_INT);
}

void uart2_send_byte(uint8_t data) {
    // wait until the TX FIFO has space (it holds 128 bytes)
    while (((REG_READ(UART2_STATUS_REG) & UART_TXFIFO_CNT_MASK) >> 16) >= 128) {
        // wait
    }
    REG_WRITE(UART2_FIFO_REG, data);   // writing here pushes the byte into the TX FIFO
}

int uart2_try_send_byte(uint8_t data) {
    uint32_t cnt = (REG_READ(UART2_STATUS_REG) & UART_TXFIFO_CNT_MASK) >> 16;
    if (cnt >= 128) {
        return 0;                       // full, byte NOT sent
    }
    REG_WRITE(UART2_FIFO_REG, data);    // pushed into TX FIFO
    return 1;                           // byte sent
}

int uart2_receive_byte(uint8_t *data) {
    // nothing waiting? return 0
    if ((REG_READ(UART2_STATUS_REG) & UART_RXFIFO_CNT_MASK) == 0) {
        return 0;
    }
    *data = (uint8_t)(REG_READ(UART2_FIFO_REG) & 0xFF);   // this read pops the byte
    return 1;
}

// int uart2_data_available(void) {
//     // number of bytes waiting in the RX FIFO (0 means none)
//     return (int)(REG_READ(UART2_STATUS_REG) & UART_RXFIFO_CNT_MASK);
// }


void uart2_send_string(const char *str){
   if(str == NULL){
    return;
   }

    while(*str!='\0'){
     uart2_send_byte(*str);
     str++;
    }
}


int uart2_timeout_occurred(void) {
    if (REG_READ(UART2_INT_RAW_REG) & UART_RXFIFO_TOUT_INT) {
        REG_WRITE(UART2_INT_CLR_REG, UART_RXFIFO_TOUT_INT);   // writing 1 to INT-CLR clears the the RX-FIFO_TOUT interrupt
        return 1;
    }
    return 0;
}

#define RX_BUF_SIZE 256

static uint8_t           rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;            // ISR writes here
static volatile uint16_t rx_tail = 0;            // main reads here
static volatile uint32_t rx_overflow_count = 0;  // bytes dropped because buffer was full

// Store one byte. Returns 1 if stored, 0 if the buffer was full.
static int rb_put(uint8_t byte) {
    uint16_t next = (rx_head + 1) % RX_BUF_SIZE;
    if (next == rx_tail) {            // full  (head +1)%size == tail
        rx_overflow_count++;
        return 0;
    }
    rx_buf[rx_head] = byte;           // write the byte first...
    rx_head = next;                   // ...then advance head
    return 1;
}

// Take one byte. Returns 1 if got a byte, 0 if the buffer was empty.
static int rb_get(uint8_t *byte) {
    if (rx_head == rx_tail) {         // empty
        return 0;
    }
    *byte = rx_buf[rx_tail];          // copy the byte out first...
    rx_tail = (rx_tail + 1) % RX_BUF_SIZE;   // ...then advance tail
    return 1;
}

// Number of bytes waiting in the buffer.
static uint16_t rb_count(void) {
    return (uint16_t)((rx_head - rx_tail + RX_BUF_SIZE) % RX_BUF_SIZE);
}

int uart2_data_available(void) {
    return (int)rb_count();
}

static volatile uint32_t uart_frm_err_count = 0;     // framing errors seen
static volatile uint32_t uart_hw_ovf_count  = 0;     // RX FIFO overflows (bytes lost in hardware)

void uart2_isr_handler(void) {
    uint32_t st = REG_READ(UART2_INT_ST_REG);        // read once, at the start

    if (st & (UART_RXFIFO_FULL_INT | UART_RXFIFO_TOUT_INT)) {
        while (REG_READ(UART2_STATUS_REG) & UART_RXFIFO_CNT_MASK) {
            uint8_t b = (uint8_t)(REG_READ(UART2_FIFO_REG) & 0xFF);
            rb_put(b);                                // if full, the byte is dropped and counted
        }
    }

    if (st & UART_RXFIFO_OVF_INT) {
        uart_hw_ovf_count++;
    }

    if (st & UART_FRM_ERR_INT) {
        uart_frm_err_count++;
    }

    REG_WRITE(UART2_INT_CLR_REG, st);                 // clear only what we saw
}

int uart2_read_byte(uint8_t *data) {
    return rb_get(data);
}
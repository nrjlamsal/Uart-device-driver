#ifndef UART2_POLLING_H
#define UART2_POLLING_H

#include <stdint.h>

/* 
   UART2 Register Addresses 
    */
#define UART2_BASE              0x3FF6E000
#define UART2_FIFO_REG          (UART2_BASE + 0x00)   // Data register
#define UART2_INT_ENA_REG       (UART2_BASE + 0x0C)   // Interrupt enable
#define UART2_INT_RAW_REG       (UART2_BASE + 0x04)   //  hardware sets bit automatically when event occured
#define UART2_INT_ST_REG        (UART2_BASE + 0x08)   // and of RAW and ENA -check to see which ISR is needed
#define UART2_INT_CLR_REG       (UART2_BASE + 0x10)   // Interrupt clear 
#define UART2_CLKDIV_REG        (UART2_BASE + 0x14)   // Baud rate divider
#define UART2_STATUS_REG        (UART2_BASE + 0x1C)   // FIFO status
#define UART2_CONF0_REG         (UART2_BASE + 0x20)   // Frame format config
#define UART2_CONF1_REG         (UART2_BASE + 0x24)   // RX timeout config




// UART2_INT_ENA_REG bit fields
#define UART_RXFIFO_FULL_INT   (1 << 0)
#define UART_FRM_ERR_INT       (1 << 3)
#define UART_RXFIFO_OVF_INT    (1 << 4)
#define UART_RXFIFO_TOUT_INT   (1 << 8)


/* 
   UART_CONF0_REG Bit Definitions
    */
#define UART_BIT_NUM_5          (0 << 2)    // 5 data bits
#define UART_BIT_NUM_6          (1 << 2)    // 6 data bits
#define UART_BIT_NUM_7          (2 << 2)    // 7 data bits
#define UART_BIT_NUM_8          (3 << 2)    // 8 data bits

#define UART_STOP_BIT_NUM_1     (1 << 4)    // 1 stop bit
#define UART_STOP_BIT_NUM_2     (3 << 4)    // 2 stop bits

#define UART_PARITY_EN          (1 << 1)    // Parity Enable bit
#define UART_PARITY             (1 << 0)    // Parity type (0=even, 1=odd)
#define UART_TICK_REF_ALWAYS_ON (1 << 27)   // Use APB clock (80 MHz)


/*
   UART_CONF1_REG Bit Definitions
    */
#define UART_RX_TOUT_EN         (1 << 31)   // Enable receive timeout
// #define UART_RX_TOUT_THRHD      (0x7F << 24) // Timeout threshold mask

/* 
   UART_STATUS_REG Bit Definitions
    */
#define UART_RXFIFO_CNT_MASK    (0xFF)      // RX FIFO byte count [7:0]
#define UART_TXFIFO_CNT_MASK    (0xFF << 16) // TX FIFO byte count [23:16]

/*   GPIO Registers  */
#define GPIO_BASE               0x3FF44000
#define GPIO_ENABLE_REG         (GPIO_BASE + 0x20)   // Output enable
#define GPIO_IN_REG             (GPIO_BASE + 0x3C)   // Input data (not used)
#define GPIO_FUNC_OUT_SEL_BASE  0x3FF44530  // Base + 4 * pin_num
#define GPIO_FUNC_IN_SEL_BASE   0x3FF44130  // Base + 4 * signal_num

/* IO_MUX Registers*/
#define IO_MUX_BASE             0x3FF49000
#define IO_MUX_GPIO16_REG       (IO_MUX_BASE + 0x4C)  // GPIO16 (RX)
#define IO_MUX_GPIO17_REG       (IO_MUX_BASE + 0x50)  // GPIO17 (TX)

/* IO_MUX Bit Definitions */
#define MCU_SEL_MASK            (0x7 << 12)        // Bits [2:0]
#define MCU_SEL_GPIO            (2 <<12)          // GPIO function
#define FUN_IE                  (1 << 9)     // Input enable

/* Clock & Reset (DPORT) Registers */
#define DPORT_BASE              0x3FF00000
#define DPORT_PERIP_CLK_EN_REG   (DPORT_BASE + 0x0C0)  // Clock enable
#define DPORT_PERIP_RST_EN_REG   (DPORT_BASE + 0x0C4)  // Reset control


/* UART2-specific bits */
#define DPORT_UART2_CLK_EN            (1 << 24)   // Bit 24: UART2 clock
#define DPORT_UART2_RST_EN            (1 << 24)   // Bit 24: UART2 reset
#define DPORT_UART_MEM_CLK_EN         (1 << 25)   // Bit 25: Shared memory clock


/* UART2 Signal Indexes (for GPIO Matrix)              */
/* Source: ESP32 TRM GPIO Matrix table, row 198        */
/* U2TXD_out = output index 198, U2RXD_in = input index 198 */
#define U2TXD_OUT_IDX           198         // U2TXD_out — write to GPIO_FUNC_OUT_SEL
#define U2RXD_IN_IDX            198         // U2RXD_in  — write to GPIO_FUNC_IN_SEL + (198*4)

/* Register Read/Write Macros */
#define REG_READ(addr)          (*(volatile uint32_t *)(uintptr_t)(addr))
#define REG_WRITE(addr, val)    (*(volatile uint32_t *)(uintptr_t)(addr) = (val))
#define REG_SET_BIT(addr, bit)  (REG_WRITE((addr), REG_READ(addr) | (bit)))
#define REG_CLR_BIT(addr, bit)  (REG_WRITE((addr), REG_READ(addr) & ~(bit)))

/* UART Configuration Structure*/

typedef struct {
    uint32_t baud_rate;
    uint8_t  data_bits;      // 5, 6, 7, 8
    uint8_t  stop_bits;      // 1, 2
    uint8_t  parity;         // 0=None, 1=Odd, 2=Even
} uart2_config_t;


#ifdef __cplusplus
extern "C" {
#endif

/*  Function Declarations */
void uart2_init(const uart2_config_t *config);//done
void uart2_send_byte(uint8_t data);//done
int  uart2_try_send_byte(uint8_t data);//done
void uart2_send_string(const char *str);//done
int  uart2_poll_receive_byte(uint8_t *data);//done
int  uart2_data_available(void);//done
void uart2_flush_rx(void);//done
void uart2_isr_handler(void);
int  uart2_read_byte(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif 
#ifndef SPI2__h
#define SPI2__h

#include <stdbool.h>
#include <stdint.h>
#include "esp/spi_regs2.h"
#include "esp/clocks.h"

/*SPI number define*/
#define SPIBUS 			0
#define HSPIBUS			1

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

/* Should already be defined in eagle_soc.h */
#ifndef CPU_CLK_FREQ
#define CPU_CLK_FREQ 80*1000000
#endif

/* Define some default SPI clock settings */
#define SPI_CLK_PREDIV 10
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ CPU_CLK_FREQ/(SPI_CLK_PREDIV*SPI_CLK_CNTDIV) // 80 / 20 = 4 MHz

/*******************************************************************************
 * spi_init:
 * Description: Wrapper to setup HSPI/SPI GPIO pins and default SPI clock
 * Parameters: spi_no - SPI (0) or HSPI (1)
*******************************************************************************/
void spi_init(uint8_t spi_no);

/*******************************************************************************
 * spi_init_gpio:
 * Description: Initialises the GPIO pins for use as SPI pins.
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - sysclk_as_spiclk:
 *       - SPI_CLK_80MHZ_NODIV (1) if using 80MHz sysclock for SPI clock.
 *       - SPI_CLK_USE_DIV (0) if using divider to get lower SPI clock speed.
*******************************************************************************/
void spi_init_gpio(uint8_t spi_no, uint8_t sysclk_as_spiclk);

/*******************************************************************************
 * spi_clock:
 * Description: sets up the control registers for the SPI clock
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - prediv: predivider value (actual division value)
 *    - cntdiv: postdivider value (actual division value).
 * Set either divider to 0 to disable all division (80MHz sysclock)
*******************************************************************************/
void spi_clock(uint8_t spi_no, uint16_t prediv, uint8_t cntdiv);

/*******************************************************************************
 * spi_tx_byte_order:
 * Description: Setup the byte order for shifting data out of buffer
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - byte_order:
 *       - SPI_BYTE_ORDER_HIGH_TO_LOW (1): Data is sent out starting with Bit31
 *         and down to Bit0
 *       - SPI_BYTE_ORDER_LOW_TO_HIGH (0): Data is sent out starting with the
 *         lowest BYTE, from MSB to LSB, followed by the second lowest BYTE,
 *         from MSB to LSB, followed by the second highest BYTE, from MSB to
 *         LSB, followed by the highest BYTE, from MSB to LSB 0xABCDEFGH would
 *         be sent as 0xGHEFCDAB
*******************************************************************************/
void spi_tx_byte_order(uint8_t spi_no, uint8_t byte_order);

/*******************************************************************************
 * spi_rx_byte_order:
 * Description: Setup the byte order for shifting data into buffer
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - byte_order:
 *       - SPI_BYTE_ORDER_HIGH_TO_LOW (1): Data is read in starting with Bit31
 *         and down to Bit0
 *       - SPI_BYTE_ORDER_LOW_TO_HIGH (0): Data is sent out starting with the
 *         lowest BYTE, from MSB to LSB, followed by the second lowest BYTE,
 *         from MSB to LSB, followed by the second highest BYTE, from MSB to
 *         LSB, followed by the highest BYTE, from MSB to LSB 0xABCDEFGH would
 *         be sent as 0xGHEFCDAB
*******************************************************************************/
void spi_rx_byte_order(uint8_t spi_no, uint8_t byte_order);

/*******************************************************************************
 * spi_mode:
 * Description: Configures SPI mode parameters for clock edge and clock polarity.
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - spi_cpha:
 *       + (0) Data is valid on clock leading edge
 *       + (1) Data is valid on clock trailing edge
 *    - spi_cpol:
 *       + (0) Clock is low when inactive
 *       + (1) Clock is high when inactive
*******************************************************************************/
void spi_mode(uint8_t spi_no, uint8_t spi_cpha,uint8_t spi_cpol);

/*******************************************************************************
 * spi_transaction:
 * Description: SPI transaction function
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - cmd_bits: actual number of command bits to transmit
 *    - cmd_data: command data to transmit
 *    - addr_bits: actual number of address bits to transmit
 *    - addr_data: address data to transmit
 *    - dout_bits: actual number of data bits to transfer out to slave
 *    - dout_data: data to send to slave
 *    - din_bits: actual number of data bits to transfer in from slave
 *    - din_data: data received from slave
 *    - dummy_bits: actual number of dummy bits to clock in to slave after
 *                  command and address have been clocked in
 * Returns:
 *    - read data: uint32 containing read in data only if RX was set
 *    - 0: something went wrong (or actual read data was 0)
 *    - 1: data sent ok (or actual read data is 1)
 *
 * Note: all data is assumed to be stored in the lower bits of the data
 * variables (for anything <32 bits).
*******************************************************************************/

void IRAM spi_transaction(uint32_t dout_bits, uint32_t dout_data, uint8_t dummy_bits);
#if 0
bool IRAM spi_transaction(uint8_t spi_no,
   uint8_t cmd_bits, uint16_t cmd_data, uint32_t addr_bits, uint32_t addr_data,
   uint32_t dout_bits, uint32_t dout_data, uint32_t din_bits, uint32_t * din_data,
   uint32_t dummy_bits
);
#endif


//Expansion Macros
#define spi_busy(spi_no) READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR




#endif

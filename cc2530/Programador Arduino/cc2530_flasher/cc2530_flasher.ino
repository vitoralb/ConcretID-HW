#include "Comunicacao.h"

/******************************************************************************
 * DEFINES
 */
// Programmer data line bitmasks (programmer I/O port 0)
#define DD                          10 // Porta 2
#define DC                          11 // Porta 3
#define RESET_N                     12 // Porta 4
// Start addresses on DUP (Increased buffer size improves performance)
#define ADDR_BUF0                   0x0000 // Buffer (512 bytes)
#define ADDR_DMA_DESC_0             0x0800 // DMA descriptors (8 bytes)
#define ADDR_DMA_DESC_1             (ADDR_DMA_DESC_0 + 8)

// DMA channels used on DUP
#define CH_DBG_TO_BUF0              0x01   // Channel 0
#define CH_BUF0_TO_FLASH            0x02   // Channel 1
// Debug commands
#define CMD_CHIP_ERASE              0x10
#define CMD_WR_CONFIG               0x19
#define CMD_RD_CONFIG               0x24
#define CMD_READ_STATUS             0x30
#define CMD_RESUME                  0x4C
#define CMD_DEBUG_INSTR_1B          (0x54|1)
#define CMD_DEBUG_INSTR_2B          (0x54|2)
#define CMD_DEBUG_INSTR_3B          (0x54|3)
#define CMD_BURST_WRITE             0x80
#define CMD_GET_CHIP_ID             0x68

// Debug status bitmasks
#define STATUS_CHIP_ERASE_BUSY_BM   0x80 // New debug interface
#define STATUS_PCON_IDLE_BM         0x40
#define STATUS_CPU_HALTED_BM        0x20
#define STATUS_PM_ACTIVE_BM         0x10
#define STATUS_HALT_STATUS_BM       0x08
#define STATUS_DEBUG_LOCKED_BM      0x04
#define STATUS_OSC_STABLE_BM        0x02
#define STATUS_STACK_OVERFLOW_BM    0x01

// DUP registers (XDATA space address)
#define DUP_DBGDATA                 0x6260  // Debug interface data buffer
#define DUP_CHVER                   0x6249  // Chip Version
#define DUP_CHIPID                  0x624A  // Chip ID
#define DUP_CHIPINFO0               0x6276  // Chip Information Byte 0
#define DUP_CHIPINFO1               0x6277  // Chip Information Byte 1
#define DUP_FCTL                    0x6270  // Flash controller
#define DUP_FADDRL                  0x6271  // Flash controller addr
#define DUP_FADDRH                  0x6272  // Flash controller addr
#define DUP_FWDATA                  0x6273  // Flash controller data buffer
#define DUP_CLKCONSTA               0x709E  // Sys clock status
#define DUP_CLKCONCMD               0x70C6  // Sys clock configuration
#define DUP_MEMCTR                  0x70C7  // Flash bank xdata mapping
#define DUP_DMA1CFGL                0x70D2  // Low byte, DMA config ch. 1
#define DUP_DMA1CFGH                0x70D3  // Hi byte , DMA config ch. 1
#define DUP_DMA0CFGL                0x70D4  // Low byte, DMA config ch. 0
#define DUP_DMA0CFGH                0x70D5  // Low byte, DMA config ch. 0
#define DUP_DMAARM                  0x70D6  // DMA arming register
// Utility macros
//! Set programmer DD line as input
#define SET_DD_INPUT()      pinMode(DD, INPUT);
//! Set programmer DD line as output
#define SET_DD_OUTPUT()     pinMode(DD, OUTPUT);
//! Low nibble of 16bit variable
#define LOBYTE(w)           ((unsigned char)(w))
//! High nibble of 16bit variable
#define HIBYTE(w)           ((unsigned char)(((unsigned short)(w) >> 8) & 0xFF))

//CRC Calculation
#define PRESET_VALUE 0xFFFF
#define POLYNOMIAL  0x8408

/******************************************************************************
 * VARIABLES
 */
//! DUP DMA descriptor
const unsigned char dma_desc_0[8] =
{
// Debug Interface -> Buffer
		HIBYTE(DUP_DBGDATA),            // src[15:8]
		LOBYTE(DUP_DBGDATA),            // src[7:0]
		HIBYTE(ADDR_BUF0),              // dest[15:8]
		LOBYTE(ADDR_BUF0),              // dest[7:0]
		0,                              // len[12:8] - filled in later
		0,                              // len[7:0]
		31,                             // trigger: DBG_BW
		0x11                            // increment destination
		};
//! DUP DMA descriptor
const unsigned char dma_desc_1[8] =
{
// Buffer -> Flash controller
		HIBYTE(ADDR_BUF0),              // src[15:8]
		LOBYTE(ADDR_BUF0),              // src[7:0]
		HIBYTE(DUP_FWDATA),             // dest[15:8]
		LOBYTE(DUP_FWDATA),             // dest[7:0]
		0,                              // len[12:8] - filled in later
		0,                              // len[7:0]
		18,                             // trigger: FLASH
		0x42,                           // increment source
		};
static unsigned char buffer[1536];

unsigned long write_addr = 0;
unsigned short write_count = 0;
unsigned long read_addr = 0;
unsigned short read_count = 0;
unsigned char chip_id = 0;
unsigned char debug_config = 0;
unsigned char chipinfo0 = 0;
unsigned char chipinfo1 = 0;
unsigned char chipid = 0;
unsigned char chver = 0;
unsigned char flash_size = 0, ram_size = 0;
unsigned char comando = 0;

/******************************************************************************
 * FUNCTIONS
 */
/**************************************************************************//**
 * @brief    Initializes the programmer by switching to 32 MHz XOSC and
 *           configuring I/O.
 *
 * @param    data    Byte to write
 *
 * @return   None.
 ******************************************************************************/
void programmer_init(void)
{
	/*    // Switch programmer (a CC2530) to 32 MHz XOSC for max performance
	 CLKCONCMD = 0x80;
	 while (CLKCONSTA != 0x80);
	 */
	// Set P0[6:0] as output low, P0[7] (RESET_N) as output high
	digitalWrite(RESET_N, HIGH);
	pinMode(DC, OUTPUT);
	pinMode(RESET_N, OUTPUT);
	pinMode(13, OUTPUT);
	digitalWrite(13, LOW);
}

/**************************************************************************//**
 * @brief    Writes a byte on the debug interface. Requires DD to be
 *           output when function is called.
 *
 * @param    data    Byte to write
 *
 * @return   None.
 ******************************************************************************/
void write_debug_byte(unsigned char data)
{
	unsigned char i;
	for (i = 0; i < 8; i++)
	{
		// Set clock high and put data on DD line
		digitalWrite(DC, HIGH);
		if (data & 0x80)
		{
			digitalWrite(DD, HIGH);
		}
		else
		{
			digitalWrite(DD, LOW);
		}

		data <<= 1;
		digitalWrite(DC, LOW); // set clock low (DUP capture flank)
	}
}

/**************************************************************************//**
 * @brief    Reads a byte from the debug interface. Requires DD to be
 *           input when function is called.
 *
 * @return   Returns the byte read.
 ******************************************************************************/
unsigned char read_debug_byte(void)
{
	unsigned char i;
	unsigned char data = 0;
	for (i = 0; i < 8; i++)
	{
		digitalWrite(DC, HIGH); // DC high
		data <<= 1;
		data |= (digitalRead(DD) & 0x01);  // Read DD line
		digitalWrite(DC, LOW);     // DC low
	}
	return data;
}

/**************************************************************************//**
 * @brief    Function waits for DUP to indicate that it is ready. The DUP will
 *           pulls DD line low when it is ready. Requires DD to be input when
 *           function is called.
 *
 * @return   Returns 0 if function timed out waiting for DD line to go low
 * @return   Returns 1 when DUP has indicated it is ready.
 ******************************************************************************/
unsigned char wait_dup_ready(void)
{
	// DUP pulls DD low when ready
	unsigned char count = 0;
	while ((digitalRead(DD) & 0x01) && count < 16)
	{
		// Clock out 8 bits before checking if DD is low again
		read_debug_byte();
		count++;
	}
	return (count == 16) ? 0 : 1;
}

/**************************************************************************//**
 * @brief    Issues a command on the debug interface. Only commands that return
 *           one output byte are supported.
 *
 * @param    cmd             Command byte
 * @param    cmd_bytes       Pointer to the array of data bytes following the
 *                           command byte [0-3]
 * @param    num_cmd_bytes   The number of data bytes (input to DUP) [0-3]
 *
 * @return   Data returned by command
 ******************************************************************************/
unsigned char debug_command(unsigned char cmd, unsigned char *cmd_bytes,
		unsigned short num_cmd_bytes)
{
	unsigned short i;
	unsigned char output = 0;

	// Make sure DD is output
	SET_DD_OUTPUT()
	;

	// Send command
	write_debug_byte(cmd);

	// Send bytes
	for (i = 0; i < num_cmd_bytes; i++)
	{
		write_debug_byte(cmd_bytes[i]);
	}

	// Set DD as input
	SET_DD_INPUT()
	;

	// Wait for data to be ready
	wait_dup_ready();

	// Read returned byte
	output = read_debug_byte();

	// Set DD as output
	SET_DD_OUTPUT()
	;

	return output;
}

/**************************************************************************//**
 * @brief    Resets the DUP into debug mode. Function assumes that
 *           the programmer I/O has already been configured using e.g.
 *           programmer_init().
 *
 * @return   None.
 ******************************************************************************/
void debug_init(void)
{
	volatile unsigned char i;
	// Send two flanks on DC while keeping RESET_N low

	// All low (incl. RESET_N)
	digitalWrite(DC, LOW);
	digitalWrite(DD, LOW);
	digitalWrite(RESET_N, LOW);

	for (i = 0; i < 10; i++)
		;   // Wait
	digitalWrite(DC, HIGH); // DC high
	digitalWrite(DC, LOW); // DC low
	digitalWrite(DC, HIGH); // DC high
	digitalWrite(DC, LOW); // DC low
	for (i = 0; i < 10; i++)
		;   // Wait
	digitalWrite(RESET_N, HIGH); // Release RESET_N
}

/**************************************************************************//**
 * @brief    Reads the chip ID over the debug interface using the
 *           GET_CHIP_ID command.
 *
 * @return   Returns the chip id returned by the DUP
 ******************************************************************************/
unsigned char read_chip_id(void)
{
	unsigned char id = 0;

	// Make sure DD is output
	SET_DD_OUTPUT()
	;

	// Send command
	write_debug_byte(CMD_GET_CHIP_ID);

	// Set DD as input
	SET_DD_INPUT()
	;

	// Wait for data to be ready
	wait_dup_ready();

	// Read ID and revision
	id = read_debug_byte(); // ID
	read_debug_byte();      // Revision (discard)

	// Set DD as output
	SET_DD_OUTPUT()
	;

	return id;
}

/**************************************************************************//**
 * @brief    Sends a block of data over the debug interface using the
 *           BURST_WRITE command.
 *
 * @param    src         Pointer to the array of input bytes
 * @param    num_bytes   The number of input bytes
 *
 * @return   None.
 ******************************************************************************/
void burst_write_block(unsigned char *src, unsigned short num_bytes)
{
	unsigned short i;

	// Make sure DD is output
	SET_DD_OUTPUT()
	;

	write_debug_byte(CMD_BURST_WRITE | HIBYTE(num_bytes) );
	write_debug_byte(LOBYTE(num_bytes) );
	for (i = 0; i < num_bytes; i++)
	{
		write_debug_byte(src[i]);
	}

	// Set DD as input
	SET_DD_INPUT()
	;

	// Wait for DUP to be ready
	wait_dup_ready();

	read_debug_byte(); // ignore output

	// Set DD as output
	SET_DD_OUTPUT()
	;
}

/**************************************************************************//**
 * @brief    Issues a CHIP_ERASE command on the debug interface and waits for it
 *           to complete.
 *
 * @return   None.
 ******************************************************************************/
void chip_erase(void)
{
	volatile unsigned char status;
	// Send command
	debug_command(CMD_CHIP_ERASE, 0, 0);

	// Wait for status bit 7 to go low
	do
	{
		status = debug_command(CMD_READ_STATUS, 0, 0);
	} while ((status & STATUS_CHIP_ERASE_BUSY_BM));
}

/**************************************************************************//**
 * @brief    Writes a block of data to the DUP's XDATA space.
 *
 * @param    address     XDATA start address
 * @param    values      Pointer to the array of bytes to write
 * @param    num_bytes   Number of bytes to write
 *
 * @return   None.
 ******************************************************************************/
void write_xdata_memory_block(unsigned short address,
		const unsigned char *values, unsigned short num_bytes)
{
	unsigned char instr[3];
	unsigned short i;

	// MOV DPTR, address
	instr[0] = 0x90;
	instr[1] = HIBYTE(address);
	instr[2] = LOBYTE(address);
	debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

	for (i = 0; i < num_bytes; i++)
	{
		// MOV A, values[i]
		instr[0] = 0x74;
		instr[1] = values[i];
		debug_command(CMD_DEBUG_INSTR_2B, instr, 2);

		// MOV @DPTR, A
		instr[0] = 0xF0;
		debug_command(CMD_DEBUG_INSTR_1B, instr, 1);

		// INC DPTR
		instr[0] = 0xA3;
		debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
	}
}

/**************************************************************************//**
 * @brief    Writes a byte to a specific address in the DUP's XDATA space.
 *
 * @param    address     XDATA address
 * @param    value       Value to write
 *
 * @return   None.
 ******************************************************************************/
void write_xdata_memory(unsigned short address, unsigned char value)
{
	unsigned char instr[3];

	// MOV DPTR, address
	instr[0] = 0x90;
	instr[1] = HIBYTE(address);
	instr[2] = LOBYTE(address);
	debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

	// MOV A, values[i]
	instr[0] = 0x74;
	instr[1] = value;
	debug_command(CMD_DEBUG_INSTR_2B, instr, 2);

	// MOV @DPTR, A
	instr[0] = 0xF0;
	debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
}

/**************************************************************************//**
 * @brief    Read a byte from a specific address in the DUP's XDATA space.
 *
 * @param    address     XDATA address
 *
 * @return   Value read from XDATA
 ******************************************************************************/
unsigned char read_xdata_memory(unsigned short address)
{
	unsigned char instr[3];

	// MOV DPTR, address
	instr[0] = 0x90;
	instr[1] = HIBYTE(address);
	instr[2] = LOBYTE(address);
	debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

	// MOVX A, @DPTR
	instr[0] = 0xE0;
	return debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
}

/**************************************************************************//**
 * @brief    Reads 1-32767 bytes from DUP's flash to a given buffer on the
 *           programmer.
 *
 * @param    bank        Flash bank to read from [0-7]
 * @param    address     Flash memory start address [0x0000 - 0x7FFF]
 * @param    values      Pointer to destination buffer.
 *
 * @return   None.
 ******************************************************************************/
void read_flash_memory_block(unsigned char bank, unsigned short flash_addr,
		unsigned short num_bytes, unsigned char *values)
{
	unsigned char instr[3];
	unsigned short i;
	unsigned short xdata_addr = (0x8000 + flash_addr);

	// 1. Map flash memory bank to XDATA address 0x8000-0xFFFF
	write_xdata_memory(DUP_MEMCTR, bank);

	// 2. Move data pointer to XDATA address (MOV DPTR, xdata_addr)
	instr[0] = 0x90;
	instr[1] = HIBYTE(xdata_addr);
	instr[2] = LOBYTE(xdata_addr);
	debug_command(CMD_DEBUG_INSTR_3B, instr, 3);

	for (i = 0; i < num_bytes; i++)
	{
		// 3. Move value pointed to by DPTR to accumulator (MOVX A, @DPTR)
		instr[0] = 0xE0;
		values[i] = debug_command(CMD_DEBUG_INSTR_1B, instr, 1);

		// 4. Increment data pointer (INC DPTR)
		instr[0] = 0xA3;
		debug_command(CMD_DEBUG_INSTR_1B, instr, 1);
	}
}

/**************************************************************************//**
 * @brief    Writes 4-2048 bytes to DUP's flash memory. Parameter \c num_bytes
 *           must be a multiple of 4.
 *
 * @param    src         Pointer to programmer's source buffer (in XDATA space)
 * @param    start_addr  FLASH memory start address [0x00000 - 0x3FFFF]
 * @param    num_bytes   Number of bytes to transfer [4-1024]
 *
 * @return   None.
 ******************************************************************************/
void write_flash_memory_block(unsigned char *src, unsigned long start_addr,
		unsigned short num_bytes)
{
	// 1. Write the 2 DMA descriptors to RAM
	write_xdata_memory_block(ADDR_DMA_DESC_0, dma_desc_0, 8);
	write_xdata_memory_block(ADDR_DMA_DESC_1, dma_desc_1, 8);

	// 2. Update LEN value in DUP's DMA descriptors
	unsigned char len[2] =
	{ HIBYTE(num_bytes), LOBYTE(num_bytes) };
	write_xdata_memory_block((ADDR_DMA_DESC_0 + 4), len, 2);  // LEN, DBG => ram
	write_xdata_memory_block((ADDR_DMA_DESC_1 + 4), len, 2); // LEN, ram => flash

	// 3. Set DMA controller pointer to the DMA descriptors
	write_xdata_memory(DUP_DMA0CFGH, HIBYTE(ADDR_DMA_DESC_0) );
	write_xdata_memory(DUP_DMA0CFGL, LOBYTE(ADDR_DMA_DESC_0) );
	write_xdata_memory(DUP_DMA1CFGH, HIBYTE(ADDR_DMA_DESC_1) );
	write_xdata_memory(DUP_DMA1CFGL, LOBYTE(ADDR_DMA_DESC_1) );

	// 4. Set Flash controller start address (wants 16MSb of 18 bit address)
	write_xdata_memory(DUP_FADDRH, HIBYTE( (start_addr>>2) ) );
	write_xdata_memory(DUP_FADDRL, LOBYTE( (start_addr>>2) ) );

	// 5. Arm DBG=>buffer DMA channel and start burst write
	write_xdata_memory(DUP_DMAARM, CH_DBG_TO_BUF0);
	burst_write_block(src, num_bytes);

	// 6. Start programming: buffer to flash
	write_xdata_memory(DUP_DMAARM, CH_BUF0_TO_FLASH);
	write_xdata_memory(DUP_FCTL, 0x06);

	// 7. Wait until flash controller is done
	while (read_xdata_memory(DUP_FCTL) & 0x80)
		;
}

/**************************************************************************//**
 * @brief    Main function.
 *
 * @return   None.
 ******************************************************************************/
void setup()
{



	Serial.begin(9600);

	/****************************************
	 * Initialize programmer
	 *****************************************/
	programmer_init();

	/****************************************
	 * Initialize debug interface
	 *****************************************/
	// Put the DUP in debug mode
	debug_init();


	if (read_chip_id() != 0xA5)
	{
		exit(1);
	}

	/****************************************
	 * Initialize DUP
	 *****************************************/
	// Switch DUP to external crystal osc. (XOSC) and wait for it to be stable.
	// This is recommended if XOSC is available during programming. If
	// XOSC is not available, comment out these two lines.
	write_xdata_memory(DUP_CLKCONCMD, 0x80);
	while (read_xdata_memory(DUP_CLKCONSTA) != 0x80)
		;


}

void loop()
{
	while (!Serial.available())
	{
		delay(100);
	}

	comando = receberComando(&Serial);

	switch (comando)
	{
	case CMD_SERIAL_GET_INFO:
		buffer[0] = chipid = read_xdata_memory(DUP_CHIPID);
		buffer[1] = chver = read_xdata_memory(DUP_CHVER);
		chipinfo0 = read_xdata_memory(DUP_CHIPINFO0);
		chipinfo1 = read_xdata_memory(DUP_CHIPINFO1);
		buffer[2] = flash_size = (chipinfo0 >> 4) & 0x05;
		buffer[3] = ram_size = (chipinfo1 & 0x07) + 1;
		enviarDados(buffer, 4, &Serial);
		break;
	case CMD_SERIAL_SET_WRITE_ADDR:
		receberDados(buffer, &Serial);
		write_addr = buffer[2];
		write_addr <<= 8;
		write_addr |= buffer[1];
		write_addr <<= 8;
		write_addr |= buffer[0];
		enviarComando(CMD_SERIAL_ACK, &Serial);
		break;
	case CMD_SERIAL_SET_WRITE_BUFFER:
		write_count = receberDados(buffer, &Serial);
		enviarComando(CMD_SERIAL_ACK, &Serial);
		break;
	case CMD_SERIAL_SET_READ_ADDR:
		break;
	case CMD_SERIAL_START_WRITE:
		// Enable DMA (Disable DMA_PAUSE bit in debug configuration)
		debug_config = 0x22;
		debug_command(CMD_WR_CONFIG, &debug_config, 1);
		// Program data (start address must be word aligned [32 bit])
		write_flash_memory_block(buffer, write_addr, write_count); // src, address, count
		enviarComando(CMD_SERIAL_ACK, &Serial);
		break;
	case CMD_SERIAL_START_READ:
		// Read 4 bytes starting at flash address 0x0100 (flash bank 0)
		// read_flash_memory_block(0, 0x0100, 4, read_data); // Bank, address, count, dest.
		break;
	case CMD_SERIAL_ERASE_CHIP:
		// Erase entire chip memory
		chip_erase();
		enviarComando(CMD_SERIAL_ACK, &Serial);
		break;
	}

}

/******************************************************************************
 *
 * Filename: eeprom.c
 *
 * Instantiation of eeprom routines
 *
 * Revision information:
 *
 * 28AUG2004	kb_admin	initial creation - adapted from Atmel sources
 * 12JAN2005	kb_admin	fixed clock generation, write polling, init
 *
 * BEGIN_KBDD_BLOCK
 * No warranty, expressed or implied, is included with this software.  It is
 * provided "AS IS" and no warranty of any kind including statutory or aspects
 * relating to merchantability or fitness for any purpose is provided.  All
 * intellectual property rights of others is maintained with the respective
 * owners.  This software is not copyrighted and is intended for reference
 * only.
 * END_BLOCK
 *
 * $FreeBSD$
 *****************************************************************************/

#include "at91rm9200_lowlevel.h"
#include "at91rm9200.h"
#include "lib.h"

/******************************* GLOBALS *************************************/


/*********************** PRIVATE FUNCTIONS/DATA ******************************/


/* Use a macro to calculate the TWI clock generator value to save code space. */
#define AT91C_TWSI_CLOCK	100000
#define TWSI_EEPROM_ADDRESS	0x50

#define TWI_CLK_BASE_DIV	((AT91C_MASTER_CLOCK/(4*AT91C_TWSI_CLOCK)) - 2)
#define SET_TWI_CLOCK	((0x00010000) | (TWI_CLK_BASE_DIV) | (TWI_CLK_BASE_DIV << 8))


/*************************** GLOBAL FUNCTIONS ********************************/


/*
 * .KB_C_FN_DEFINITION_START
 * void InitEEPROM(void)
 *  This global function initializes the EEPROM interface (TWI).  Intended
 * to be called a single time.
 * .KB_C_FN_DEFINITION_END
 */
void
InitEEPROM(void)
{

	AT91PS_TWI twiPtr = (AT91PS_TWI)AT91C_BASE_TWI;

	AT91PS_PIO pPio = (AT91PS_PIO)AT91C_BASE_PIOA;
	AT91PS_PMC pPMC = (AT91PS_PMC)AT91C_BASE_PMC;

	pPio->PIO_ASR = AT91C_PA25_TWD | AT91C_PA26_TWCK;
	pPio->PIO_PDR = AT91C_PA25_TWD | AT91C_PA26_TWCK;

	pPio->PIO_MDDR = ~AT91C_PA25_TWD;
	pPio->PIO_MDER = AT91C_PA25_TWD;

	pPMC->PMC_PCER = 1u << AT91C_ID_TWI;

	twiPtr->TWI_IDR = 0xffffffffu;
	twiPtr->TWI_CR = AT91C_TWI_SWRST;
	twiPtr->TWI_CR = AT91C_TWI_MSEN | AT91C_TWI_SVDIS;

	twiPtr->TWI_CWGR = SET_TWI_CLOCK;
}


/*
 * .KB_C_FN_DEFINITION_START
 * void ReadEEPROM(unsigned ee_addr, char *data_addr, unsigned size)
 *  This global function reads data from the eeprom at ee_addr storing data
 * to data_addr for size bytes.  Assume the TWI has been initialized.
 * This function does not utilize the page read mode to simplify the code.
 * .KB_C_FN_DEFINITION_END
 */
void
ReadEEPROM(unsigned ee_addr, char *data_addr, unsigned size)
{
	const AT91PS_TWI 	twiPtr = AT91C_BASE_TWI;
	unsigned int status;

	status = twiPtr->TWI_SR;
	status = twiPtr->TWI_RHR;

	// Set the TWI Master Mode Register
	twiPtr->TWI_MMR = (TWSI_EEPROM_ADDRESS << 16) |
	    AT91C_TWI_IADRSZ_2_BYTE | AT91C_TWI_MREAD;

	// Set TWI Internal Address Register
	twiPtr->TWI_IADR = ee_addr;

	// Start transfer
	twiPtr->TWI_CR = AT91C_TWI_START;

	status = twiPtr->TWI_SR;

	while (size-- > 1){

		// Wait RHR Holding register is full
		while (!(twiPtr->TWI_SR & AT91C_TWI_RXRDY))
			continue;

		// Read byte
		*(data_addr++) = twiPtr->TWI_RHR;
	}

	twiPtr->TWI_CR = AT91C_TWI_STOP;

	status = twiPtr->TWI_SR;

	// Wait transfer is finished
	while (!(twiPtr->TWI_SR & AT91C_TWI_TXCOMP))
		continue;

	// Read last byte
	*data_addr = twiPtr->TWI_RHR;
}


/*
 * .KB_C_FN_DEFINITION_START
 * void WriteEEPROM(unsigned ee_addr, char *data_addr, unsigned size)
 *  This global function writes data to the eeprom at ee_addr using data
 * from data_addr for size bytes.  Assume the TWI has been initialized.
 * This function does not utilize the page write mode as the write time is
 * much greater than the time required to access the device for byte-write
 * functionality.  This allows the function to be much simpler.
 * .KB_C_FN_DEFINITION_END
 */
void
WriteEEPROM(unsigned ee_addr, char *data_addr, unsigned size)
{
	const AT91PS_TWI 	twiPtr = AT91C_BASE_TWI;
	unsigned		status;
	char			test_data;

	while (size--) {
		if (!(ee_addr & 0x3f))
			putchar('.');

		// Set the TWI Master Mode Register
		twiPtr->TWI_MMR = ((TWSI_EEPROM_ADDRESS << 16) |
		    AT91C_TWI_IADRSZ_2_BYTE) & ~AT91C_TWI_MREAD;

		// Set TWI Internal Address Register
		twiPtr->TWI_IADR = ee_addr++;

		status = twiPtr->TWI_SR;

		twiPtr->TWI_THR = *(data_addr++);

		twiPtr->TWI_CR = AT91C_TWI_START;

		// Wait transfer is finished
		while (!(twiPtr->TWI_SR & AT91C_TWI_TXRDY))
			continue;

		twiPtr->TWI_CR = AT91C_TWI_STOP;

		status = twiPtr->TWI_SR;

		// Wait transfer is finished
		while (!(twiPtr->TWI_SR & AT91C_TWI_TXCOMP))
			continue;

		// wait for write operation to complete
		ReadEEPROM(ee_addr, &test_data, 1);
	}

	putchar('\r');
	putchar('\n');
}

/* $FreeBSD$ */

/*
 * This is part of the Driver for Video Capture Cards (Frame grabbers)
 * and TV Tuner cards using the Brooktree Bt848, Bt848A, Bt849A, Bt878, Bt879
 * chipset.
 * Copyright Roger Hardiman and Amancio Hasty.
 *
 * bktr_card : This deals with identifying TV cards.
 *               trying to find the card make and model of card.
 *               trying to find the type of tuner fitted.
 *               reading the configuration EEPROM.
 *               locating i2c devices.
 *
 */

/*
 * 1. Redistributions of source code must retain the
 * Copyright (c) 1997 Amancio Hasty, 1999 Roger Hardiman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Amancio Hasty and
 *      Roger Hardiman
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <machine/clock.h>      /* for DELAY */

#include <pci/pcivar.h>

#include <machine/ioctl_meteor.h>
#include <machine/ioctl_bt848.h>        /* extensions to ioctl_meteor.h */
#include <dev/bktr/bktr_reg.h>
#include <dev/bktr/bktr_core.h>
#include <dev/bktr/bktr_tuner.h>
#include <dev/bktr/bktr_card.h>
#include <dev/bktr/bktr_audio.h>

/* Various defines */
#define HAUP_REMOTE_INT_WADDR   0x30
#define HAUP_REMOTE_INT_RADDR   0x31
 
#define HAUP_REMOTE_EXT_WADDR   0x34
#define HAUP_REMOTE_EXT_RADDR   0x35

/* address of BTSC/SAP decoder chip */
#define TDA9850_WADDR           0xb6 
#define TDA9850_RADDR           0xb7
 
/* address of MSP3400C chip */
#define MSP3400C_WADDR          0x80
#define MSP3400C_RADDR          0x81
 
 
/* EEProm (128 * 8) on an STB card */
#define X24C01_WADDR            0xae
#define X24C01_RADDR            0xaf
 
 
/* EEProm (256 * 8) on a Hauppauge card */
/* and on most BT878s cards to store the sub-system vendor id */
#define PFC8582_WADDR           0xa0
#define PFC8582_RADDR		0xa1

#if BROOKTREE_SYSTEM_DEFAULT == BROOKTREE_PAL
#define DEFAULT_TUNER   PHILIPS_PALI
#else
#define DEFAULT_TUNER   PHILIPS_NTSC
#endif




/*
 * the data for each type of card
 *
 * Note:
 *   these entried MUST be kept in the order defined by the CARD_XXX defines!
 */
static const struct CARDTYPE cards[] = {

	{  CARD_UNKNOWN,			/* the card id */
	  "Unknown",				/* the 'name' */
	   NULL,				/* the tuner */
	   0,					/* the tuner i2c address */
	   0,					/* dbx unknown */
	   0,
	   0,					/* EEProm unknown */
	   0,					/* EEProm unknown */
	   { 0, 0, 0, 0, 0 },
	   0 },					/* GPIO mask */

	{  CARD_MIRO,				/* the card id */
	  "Miro TV",				/* the 'name' */
	   NULL,				/* the tuner */
	   0,					/* the tuner i2c address */
	   0,					/* dbx unknown */
	   0,
	   0,					/* EEProm unknown */
	   0,					/* size unknown */
	   { 0x02, 0x01, 0x00, 0x0a, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

	{  CARD_HAUPPAUGE,			/* the card id */
	  "Hauppauge WinCast/TV",		/* the 'name' */
	   NULL,				/* the tuner */
	   0,					/* the tuner i2c address */
	   0,					/* dbx is optional */
	   0,
	   PFC8582_WADDR,			/* EEProm type */
	   (u_char)(256 / EEPROMBLOCKSIZE),	/* 256 bytes */
	   { 0x00, 0x02, 0x01, 0x04, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

	{  CARD_STB,				/* the card id */
	  "STB TV/PCI",				/* the 'name' */
	   NULL,				/* the tuner */
	   0,					/* the tuner i2c address */
	   0,					/* dbx is optional */
	   0,
	   X24C01_WADDR,			/* EEProm type */
	   (u_char)(128 / EEPROMBLOCKSIZE),	/* 128 bytes */
	   { 0x00, 0x01, 0x02, 0x02, 1 }, 	/* audio MUX values */
	   0x0f },				/* GPIO mask */

	{  CARD_INTEL,				/* the card id */
	  "Intel Smart Video III/VideoLogic Captivator PCI", /* the 'name' */
	   NULL,				/* the tuner */
	   0,					/* the tuner i2c address */
	   0,
	   0,
	   0,
	   0,
	   { 0, 0, 0, 0, 0 }, 			/* audio MUX values */
	   0x00 },				/* GPIO mask */

	{  CARD_IMS_TURBO,			/* the card id */
	  "IMS TV Turbo",			/* the 'name' */
	   NULL,				/* the tuner */
	   0,					/* the tuner i2c address */
	   0,					/* dbx is optional */
	   0,
	   PFC8582_WADDR,			/* EEProm type */
	   (u_char)(256 / EEPROMBLOCKSIZE),	/* 256 bytes */
	   { 0x01, 0x02, 0x01, 0x00, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

        {  CARD_AVER_MEDIA,			/* the card id */
          "AVer Media TV/FM",                   /* the 'name' */
           NULL,                                /* the tuner */
	   0,					/* the tuner i2c address */
           0,                                   /* dbx is optional */
           0,
           0,                                   /* EEProm type */
           0,                                   /* EEProm size */
           { 0x0c, 0x00, 0x0b, 0x0b, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

        {  CARD_OSPREY,				/* the card id */
          "MMAC Osprey",                   	/* the 'name' */
           NULL,                                /* the tuner */
	   0,					/* the tuner i2c address */
           0,                                   /* dbx is optional */
           0,
	   PFC8582_WADDR,			/* EEProm type */
	   (u_char)(256 / EEPROMBLOCKSIZE),	/* 256 bytes */
           { 0x00, 0x00, 0x00, 0x00, 0 },	/* audio MUX values */
	   0 },					/* GPIO mask */

        {  CARD_NEC_PK,                         /* the card id */
          "NEC PK-UG-X017",                     /* the 'name' */
           NULL,                                /* the tuner */
           0,                                   /* the tuner i2c address */
           0,                                   /* dbx is optional */
           0,
           0,                                   /* EEProm type */
           0,                                   /* EEProm size */
           { 0x01, 0x02, 0x01, 0x00, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

        {  CARD_IO_GV,                          /* the card id */
          "I/O DATA GV-BCTV2/PCI",              /* the 'name' */
           NULL,                                /* the tuner */
           0,                                   /* the tuner i2c address */
           0,                                   /* dbx is optional */
           0,
           0,                                   /* EEProm type */
           0,                                   /* EEProm size */
           { 0x00, 0x00, 0x00, 0x00, 1 },	/* Has special MUX handler */
	   0x0f },				/* GPIO mask */

        {  CARD_FLYVIDEO,			/* the card id */
          "FlyVideo",				/* the 'name' */
           NULL,				/* the tuner */
           0,					/* the tuner i2c address */
           0,					/* dbx is optional */
           0,					/* msp34xx is optional */
	   0xac,				/* EEProm type */
	   (u_char)(256 / EEPROMBLOCKSIZE),	/* 256 bytes */
           { 0x000, 0x800, 0x400, 0x8dff00, 1 },/* audio MUX values */
	   0x8dff00 },				/* GPIO mask */

	{  CARD_ZOLTRIX,			/* the card id */
	  "Zoltrix",				/* the 'name' */
           NULL,				/* the tuner */
           0,					/* the tuner i2c address */
           0,					/* dbx is optional */
           0,					/* msp34xx is optional */
	   0,					/* EEProm type */
	   0,					/* EEProm size */
	   { 0x04, 0x01, 0x00, 0x0a, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

	{  CARD_KISS,				/* the card id */
	  "KISS TV/FM PCI",			/* the 'name' */
           NULL,				/* the tuner */
           0,					/* the tuner i2c address */
           0,					/* dbx is optional */
           0,					/* msp34xx is optional */
	   0,					/* EEProm type */
	   0,					/* EEProm size */
	   { 0x0c, 0x00, 0x0b, 0x0b, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

	{  CARD_VIDEO_HIGHWAY_XTREME,		/* the card id */
	  "Video Highway Xtreme",		/* the 'name' */
           NULL,				/* the tuner */
           0,					/* the tuner i2c address */
           0,					/* dbx is optional */
           0,					/* msp34xx is optional */
	   0,					/* EEProm type */
	   0,					/* EEProm size */
	   { 0x00, 0x02, 0x01, 0x04, 1 },	/* audio MUX values */
	   0x0f },				/* GPIO mask */

};

struct bt848_card_sig bt848_card_signature[1]= {
  /* IMS TURBO TV : card 5 */
    {  5,9, {00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 02, 00, 00, 00}}


};


/*
 * Write to the configuration EEPROM on the card.
 * This is dangerous and will mess up your card. Therefore it is not
 * implemented.
 */
int      
writeEEProm( bktr_ptr_t bktr, int offset, int count, u_char *data )
{
        return( -1 );
}

/*
 * Read the contents of the configuration EEPROM on the card.
 * (This is not fitted to all makes of card. All Hauppuage cards have them
 * and so do newer Bt878 based cards.
 */
int
readEEProm( bktr_ptr_t bktr, int offset, int count, u_char *data )
{
	int	x;
	int	addr;
	int	max;
	int	byte;

	/* get the address of the EEProm */
	addr = (int)(bktr->card.eepromAddr & 0xff);
	if ( addr == 0 )
		return( -1 );

	max = (int)(bktr->card.eepromSize * EEPROMBLOCKSIZE);
	if ( (offset + count) > max )
		return( -1 );

	/* set the start address */
	if ( i2cWrite( bktr, addr, offset, -1 ) == -1 )
		return( -1 );

	/* the read cycle */
	for ( x = 0; x < count; ++x ) {
		if ( (byte = i2cRead( bktr, (addr | 1) )) == -1 )
			return( -1 );
		data[ x ] = byte;
	}

	return( 0 );
}


#define ABSENT		(-1)

/*
 * get a signature of the card
 * read all 128 possible i2c read addresses from 0x01 thru 0xff
 * build a bit array with a 1 bit for each i2c device that responds
 *
 * XXX FIXME: use offset & count args
 */
int
signCard( bktr_ptr_t bktr, int offset, int count, u_char* sig )
{
	int	x;

	for ( x = 0; x < 16; ++x )
		sig[ x ] = 0;

	for ( x = 0; x < count; ++x ) {
		if ( i2cRead( bktr, (2 * x) + 1 ) != ABSENT ) {
			sig[ x / 8 ] |= (1 << (x % 8) );
		}
	}

	return( 0 );
}


/*
 * check_for_i2c_devices.
 * Some BT848 cards have no tuner and no additional i2c devices
 * eg stereo decoder. These are used for video conferencing or capture from
 * a video camera. (eg VideoLogic Captivator PCI, Intel SmartCapture card).
 *
 * Determine if there are any i2c devices present. There are none present if
 *  a) reading from all 128 devices returns ABSENT (-1) for each one
 *     (eg VideoLogic Captivator PCI with BT848)
 *  b) reading from all 128 devices returns 0 for each one
 *     (eg VideoLogic Captivator PCI rev. 2F with BT848A)
 */
static int check_for_i2c_devices( bktr_ptr_t bktr ){
  int x, temp_read;
  int i2c_all_0 = 1;
  int i2c_all_absent = 1;
  for ( x = 0; x < 128; ++x ) {
    temp_read = i2cRead( bktr, (2 * x) + 1 );
    if (temp_read != 0)      i2c_all_0 = 0;
    if (temp_read != ABSENT) i2c_all_absent = 0;
  }

  if ((i2c_all_0) || (i2c_all_absent)) return 0;
  else return 1;
}


/*
 * Temic/Philips datasheets say tuners can be at i2c addresses 0xc0, 0xc2,
 * 0xc4 or 0xc6, settable by links on the tuner.
 * Determine the actual address used on the TV card by probing read addresses.
 */
static int locate_tuner_address( bktr_ptr_t bktr) {
  if (i2cRead( bktr, 0xc1) != ABSENT) return 0xc0;
  if (i2cRead( bktr, 0xc3) != ABSENT) return 0xc2;
  if (i2cRead( bktr, 0xc5) != ABSENT) return 0xc4;
  if (i2cRead( bktr, 0xc7) != ABSENT) return 0xc6;
  return -1; /* no tuner found */
}

 
/*
 * Search for a configuration EEPROM on the i2c bus by looking at i2c addresses
 * where EEPROMs are usually found.
 * On some cards, the EEPROM appears in several locations, but all in the
 * range 0xa0 to 0xae.
 */
static int locate_eeprom_address( bktr_ptr_t bktr) {
  if (i2cRead( bktr, 0xa0) != ABSENT) return 0xa0;
  if (i2cRead( bktr, 0xac) != ABSENT) return 0xac;
  if (i2cRead( bktr, 0xae) != ABSENT) return 0xae;
  return -1; /* no eeprom found */
}


/*
 * determine the card brand/model
 * OVERRIDE_CARD, OVERRIDE_TUNER, OVERRIDE_DBX and OVERRIDE_MSP
 * can be used to select a specific device, regardless of the
 * autodetection and i2c device checks.
 *
 * The scheme used for probing cards faces these problems:
 *  It is impossible to work out which type of tuner is actually fitted,
 *  (the driver cannot tell if the Tuner is PAL or NTSC, Temic or Philips)
 *  It is impossible to determine what audio-mux hardware is connected.
 *  It is impossible to determine if there is extra hardware connected to the
 *  GPIO pins  (eg radio chips or MSP34xx reset logic)
 *
 * However some makes of card (eg Hauppauge) come with a configuration eeprom
 * which tells us the make of the card. Most eeproms also tell us the
 * tuner type and other features of the the cards.
 *
 * The current probe code works as follows
 * A) If the card uses a Bt878/879:
 *   1) Read the sub-system vendor id from the configuration EEPROM.
 *      Select the required tuner, audio mux arrangement and any other
 *      onboard features. If this fails, move to step B.
 * B) If it card uses a Bt848, 848A, 849A or an unknown Bt878/879:
 *   1) Look for I2C devices. If there are none fitted, it is an Intel or
 *      VideoLogic cards.
 *   2) Look for a configuration EEPROM.
 *   2a) If there is one at I2C address 0xa0 it may be
 *       a Hauppauge or an Osprey. Check the EEPROM contents to determine which
 *       one it is. For Hauppauge, select the tuner type and audio hardware.
 *   2b) If there is an EEPROM at I2C address 0xa8 it will be an STB card.
 *       We still have to guess on the tuner type.
 *              
 * C) If we do not know the card type from (A) or (B), guess at the tuner
 *    type based on the I2C address of the tuner.
 *
 * D) After determining the Tuner Type, we probe the i2c bus for other
 *    devices at known locations, eg IR-Remote Control, MSP34xx and TDA
 *    stereo chips.
 */


/*
 * These are the sub-system vendor ID codes stored in the
 * configuration EEPROM used on Bt878/879 cards. They should match the
 * number assigned to the company by the PCI Special Interest Group
 */
#define VENDOR_AVER_MEDIA 0x1461
#define VENDOR_HAUPPAUGE  0x0070
#define VENDOR_FLYVIDEO   0x1851
#define VENDOR_STB        0x10B4


void
probeCard( bktr_ptr_t bktr, int verbose, int unit )
{
	int		card, i,j, card_found;
	int		status;
	bt848_ptr_t	bt848;
	u_char 		probe_signature[128], *probe_temp;
        int   		any_i2c_devices;
	u_char 		eeprom[256];
	u_char 		tuner_code = 0;
	int 		tuner_i2c_address = -1;
	int 		eeprom_i2c_address = -1;

	bt848 = bktr->base;

	/* Select all GPIO bits as inputs */
	bt848->gpio_out_en = 0;
	if (bootverbose)
	    printf("bktr: GPIO is 0x%08x\n", bt848->gpio_data);

#ifdef HAUPPAUGE_MSP_RESET
	/* Reset the MSP34xx audio chip. This resolves bootup card
	 * detection problems with old Bt848 based Hauppauge cards with
	 * MSP34xx stereo audio chips. This must be user enabled because
	 * at this point the probe function does not know the card type. */
        bt848->gpio_out_en = bt848->gpio_out_en | (1<<5);
        bt848->gpio_data   = bt848->gpio_data | (1<<5);  /* write '1' */
        DELAY(2500); /* wait 2.5ms */
        bt848->gpio_data   = bt848->gpio_data & ~(1<<5); /* write '0' */
        DELAY(2500); /* wait 2.5ms */
        bt848->gpio_data   = bt848->gpio_data | (1<<5);  /* write '1' */
        DELAY(2500); /* wait 2.5ms */
#endif

	/* Check for the presence of i2c devices */
        any_i2c_devices = check_for_i2c_devices( bktr );


	/* Check for a user specified override on the card selection */
#if defined( OVERRIDE_CARD )
	bktr->card = cards[ (card = OVERRIDE_CARD) ];
	goto checkEEPROM;
#endif
	if (bktr->bt848_card != -1 ) {
	  bktr->card = cards[ (card = bktr->bt848_card) ];
	  goto checkEEPROM;
	}


	/* No override, so try and determine the make of the card */

        /* On BT878/879 cards, read the sub-system vendor id */
	/* This identifies the manufacturer of the card and the model */
	/* In theory this can be read from PCI registers but this does not */
	/* appear to work on the FlyVideo 98. Hauppauge also warned that */
	/* the PCI registers are sometimes not loaded correctly. */
	/* Therefore, I will read the sub-system vendor ID from the EEPROM */
	/* (just like the Bt878 does during power up initialisation) */

        if ((bktr->id==BROOKTREE_878) || (bktr->id==BROOKTREE_879)) {
	    /* Try and locate the EEPROM */
	    eeprom_i2c_address = locate_eeprom_address( bktr );
	    if (eeprom_i2c_address != -1) {

                unsigned int subsystem_vendor_id; /* vendors PCI-SIG ID */
                unsigned int subsystem_id;        /* board model number */
		unsigned int byte_252, byte_253, byte_254, byte_255;

		bktr->card = cards[ (card = CARD_UNKNOWN) ];
		bktr->card.eepromAddr = eeprom_i2c_address;
		bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);

	        readEEProm(bktr, 0, 256, (u_char *) &eeprom );
                byte_252 = (unsigned int)eeprom[252];
                byte_253 = (unsigned int)eeprom[253];
                byte_254 = (unsigned int)eeprom[254];
                byte_255 = (unsigned int)eeprom[255];
                
                subsystem_id        = (byte_252 << 8) | byte_253;
                subsystem_vendor_id = (byte_254 << 8) | byte_255;

	        if ( bootverbose ) 
	            printf("subsytem 0x%04x 0x%04x\n",subsystem_vendor_id,
		                                  subsystem_id);

                if (subsystem_vendor_id == VENDOR_AVER_MEDIA) {
                    bktr->card = cards[ (card = CARD_AVER_MEDIA) ];
		    bktr->card.eepromAddr = eeprom_i2c_address;
		    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
                    goto checkTuner;
                }

                if (subsystem_vendor_id == VENDOR_HAUPPAUGE) {
                    bktr->card = cards[ (card = CARD_HAUPPAUGE) ];
		    bktr->card.eepromAddr = eeprom_i2c_address;
		    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
                    goto checkTuner;
                }

                if (subsystem_vendor_id == VENDOR_FLYVIDEO) {
                    bktr->card = cards[ (card = CARD_FLYVIDEO) ];
		    bktr->card.eepromAddr = eeprom_i2c_address;
		    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
                    goto checkTuner;
                }

                if (subsystem_vendor_id == VENDOR_STB) {
                    bktr->card = cards[ (card = CARD_STB) ];
		    bktr->card.eepromAddr = eeprom_i2c_address;
		    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
                    goto checkTuner;
                }

                /* Vendor is unknown. We will use the standard probe code */
		/* which may not give best results */
                printf("Warning - card vendor 0x%04x (model 0x%04x) unknown.\n",subsystem_vendor_id,subsystem_id);
            }
	    else
	    {
                printf("Card has no configuration EEPROM. Cannot determine card make.\n");
	    }
	} /* end of bt878/bt879 card detection code */

	/* If we get to this point, we must have a Bt848/848A/849A card */
	/* or a Bt878/879 with an unknown subsystem vendor id */
        /* Try and determine the make of card by clever i2c probing */

   	/* Check for i2c devices. If none, move on */
	if (!any_i2c_devices) {
		bktr->card = cards[ (card = CARD_INTEL) ];
		bktr->card.eepromAddr = 0;
		bktr->card.eepromSize = 0;
		goto checkTuner;
	}

        /* Look for Hauppauge, STB and Osprey cards by the presence */
	/* of an EEPROM */
        /* Note: Bt878 based cards also use EEPROMs so we can only do this */
        /* test on BT848/848A and 849A based cards. */
	if ((bktr->id==BROOKTREE_848)  ||
	    (bktr->id==BROOKTREE_848A) ||
	    (bktr->id==BROOKTREE_849A)) {

            /* At i2c address 0xa0, look for Hauppauge and Osprey cards */
            if ( (status = i2cRead( bktr, PFC8582_RADDR )) != ABSENT ) {

		    /* Read the eeprom contents */
		    bktr->card = cards[ (card = CARD_UNKNOWN) ];
		    bktr->card.eepromAddr = PFC8582_WADDR;
		    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
	            readEEProm(bktr, 0, 128, (u_char *) &eeprom );

		    /* For Hauppauge, check the EEPROM begins with 0x84 */
		    if (eeprom[0] == 0x84) {
                            bktr->card = cards[ (card = CARD_HAUPPAUGE) ];
			    bktr->card.eepromAddr = PFC8582_WADDR;
			    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
                            goto checkTuner;
		    }

		    /* For Osprey, check the EEPROM begins with "MMAC" */
		    if (  (eeprom[0] == 'M') &&(eeprom[1] == 'M')
			&&(eeprom[2] == 'A') &&(eeprom[3] == 'C')) {
                            bktr->card = cards[ (card = CARD_OSPREY) ];
			    bktr->card.eepromAddr = PFC8582_WADDR;
			    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
                            goto checkTuner;
		    }
		    printf("Warning: Unknown card type. EEPROM data not recognised\n");
		    printf("%x %x %x %x\n",eeprom[0],eeprom[1],eeprom[2],eeprom[3]);
            }

            /* look for an STB card */
            if ( (status = i2cRead( bktr, X24C01_RADDR )) != ABSENT ) {
                    bktr->card = cards[ (card = CARD_STB) ];
		    bktr->card.eepromAddr = X24C01_WADDR;
		    bktr->card.eepromSize = (u_char)(128 / EEPROMBLOCKSIZE);
                    goto checkTuner;
            }

	}

	signCard( bktr, 1, 128, (u_char *)  &probe_signature );

	if (bootverbose) {
	  printf("card signature \n");
	  for (j = 0; j < Bt848_MAX_SIGN; j++) {
	    printf(" %02x ", probe_signature[j]);
	  }
	  printf("\n\n");
	}
	for (i = 0;
	     i < (sizeof bt848_card_signature)/ sizeof (struct bt848_card_sig);
	     i++ ) {

	  card_found = 1;
	  probe_temp = (u_char *) &bt848_card_signature[i].signature;

	  for (j = 0; j < Bt848_MAX_SIGN; j++) {
	    if ((probe_temp[j] & 0xf) != (probe_signature[j] & 0xf)) {
	      card_found = 0;
	      break;
	    }

	  }
	  if (card_found) {
	    bktr->card = cards[ card = bt848_card_signature[i].card];
	    select_tuner( bktr, bt848_card_signature[i].tuner );
	    eeprom_i2c_address = locate_eeprom_address( bktr );
	    if (eeprom_i2c_address != -1) {
		bktr->card.eepromAddr = eeprom_i2c_address;
		bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
	    } else {
		bktr->card.eepromAddr = 0;
		bktr->card.eepromSize = 0;
	    }
	    goto checkDBX;
	  }
	}

	/* We do not know the card type. Default to Miro */
	bktr->card = cards[ (card = CARD_MIRO) ];


checkEEPROM:
	/* look for a configuration eeprom */
	eeprom_i2c_address = locate_eeprom_address( bktr );
	if (eeprom_i2c_address != -1) {
	    bktr->card.eepromAddr = eeprom_i2c_address;
	    bktr->card.eepromSize = (u_char)(256 / EEPROMBLOCKSIZE);
	} else {
	    bktr->card.eepromAddr = 0;
	    bktr->card.eepromSize = 0;
	}


checkTuner:

	/* look for a tuner */
	tuner_i2c_address = locate_tuner_address( bktr );
	if ( tuner_i2c_address == -1 ) {
		select_tuner( bktr, NO_TUNER );
		goto checkDBX;
	}

#if defined( OVERRIDE_TUNER )
	select_tuner( bktr, OVERRIDE_TUNER );
	goto checkDBX;
#endif
	if (bktr->bt848_tuner != -1 ) {
	  select_tuner( bktr, bktr->bt848_tuner & 0xff );
	  goto checkDBX;
	}

	/* Check for i2c devices */
	if (!any_i2c_devices) {
		select_tuner( bktr, NO_TUNER );
		goto checkDBX;
	}

	/* differentiate type of tuner */

	switch (card) {
	case CARD_MIRO:
	    switch (((bt848->gpio_data >> 10)-1)&7) {
	    case 0: select_tuner( bktr, TEMIC_PAL ); break;
	    case 1: select_tuner( bktr, PHILIPS_PAL ); break;
	    case 2: select_tuner( bktr, PHILIPS_NTSC ); break;
	    case 3: select_tuner( bktr, PHILIPS_SECAM ); break;
	    case 4: select_tuner( bktr, NO_TUNER ); break;
	    case 5: select_tuner( bktr, PHILIPS_PALI ); break;
	    case 6: select_tuner( bktr, TEMIC_NTSC ); break;
	    case 7: select_tuner( bktr, TEMIC_PALI ); break;
	    }
	    goto checkDBX;
	    break;

	case CARD_HAUPPAUGE:
	    /* Hauppauge kindly supplied the following Tuner Table */
	    /* FIXME: I think the tuners the driver selects for types */
	    /* 0x08 and 0x15 may be incorrect but no one has complained. */
	    /*
   	    	ID Tuner Model          Format         	We select Format
	   	 0 NONE               
		 1 EXTERNAL             
		 2 OTHER                
		 3 Philips FI1216       BG 
		 4 Philips FI1216MF     BGLL' 
		 5 Philips FI1236       MN 		PHILIPS_NTSC
		 6 Philips FI1246       I 		PHILIPS_PALI
		 7 Philips FI1256       DK 
		 8 Philips FI1216 MK2   BG 		PHILIPS_PALI
		 9 Philips FI1216MF MK2 BGLL' 
		 a Philips FI1236 MK2   MN 		PHILIPS_NTSC
		 b Philips FI1246 MK2   I 		PHILIPS_PALI
		 c Philips FI1256 MK2   DK 
		 d Temic 4032FY5        NTSC		TEMIC_NTSC
		 e Temic 4002FH5        BG		TEMIC_PAL
		 f Temic 4062FY5        I 		TEMIC_PALI
		10 Philips FR1216 MK2   BG 
		11 Philips FR1216MF MK2 BGLL' 
		12 Philips FR1236 MK2   MN 		PHILIPS_FR1236_NTSC
		13 Philips FR1246 MK2   I 
		14 Philips FR1256 MK2   DK 
		15 Philips FM1216       BG 		PHILIPS_FR1216_PAL
		16 Philips FM1216MF     BGLL' 
		17 Philips FM1236       MN 		PHILIPS_FR1236_NTSC
		18 Philips FM1246       I 
		19 Philips FM1256       DK 
		1a Temic 4036FY5        MN - FI1236 MK2 clone
		1b Samsung TCPN9082D    MN 
		1c Samsung TCPM9092P    Pal BG/I/DK 
		1d Temic 4006FH5        BG 		PHILIPS_PALI clone
		1e Samsung TCPN9085D    MN/Radio 
		1f Samsung TCPB9085P    Pal BG/I/DK / Radio 
		20 Samsung TCPL9091P    Pal BG & Secam L/L' 
		21 Temic 4039FY5        NTSC Radio

	    */



	    /* Determine the model number from the eeprom */
	    if (bktr->card.eepromAddr != 0) {
		u_int model;
		u_int revision;

		readEEProm(bktr, 0, 128, (u_char *) &eeprom );

		model    = (eeprom[12] << 8  | eeprom[11]);
		revision = (eeprom[15] << 16 | eeprom[14] << 8 | eeprom[13]);
		if (verbose)
		    printf("bktr%d: Hauppauge Model %d %c%c%c%c\n",
			unit,
			model,
			((revision >> 18) & 0x3f) + 32,
			((revision >> 12) & 0x3f) + 32,
			((revision >>  6) & 0x3f) + 32,
			((revision >>  0) & 0x3f) + 32 );

	        /* Determine the tuner type from the eeprom */
		tuner_code = eeprom[9];
		switch (tuner_code) {

		  case 0x5:
                  case 0x0a:
	          case 0x1a:
		    select_tuner( bktr, PHILIPS_NTSC );
		    goto checkDBX;

                  case 0x12:
	          case 0x17:
		    select_tuner( bktr, PHILIPS_FR1236_NTSC );
		    goto checkDBX;

		  case 0x6:
	          case 0x8:
	          case 0xb:
	          case 0x1d:
		    select_tuner( bktr, PHILIPS_PALI );
		    goto checkDBX;

	          case 0xd:
		    select_tuner( bktr, TEMIC_NTSC );
		    goto checkDBX;

                  case 0xe:
		    select_tuner( bktr, TEMIC_PAL );
		    goto checkDBX;

	          case 0xf:
		    select_tuner( bktr, TEMIC_PALI );
		    goto checkDBX;

                  case 0x15:
		    select_tuner( bktr, PHILIPS_FR1216_PAL );
		    goto checkDBX;

	          default :
		    printf("Warning - Unknown Hauppauge Tuner 0x%x\n",tuner_code);
		}
	    }
	    break;


	case CARD_AVER_MEDIA:
	    /* AVerMedia kindly supplied some details of their EEPROM contents
	     * which allow us to auto select the Tuner Type.
	     * Only the newer AVerMedia cards actually have an EEPROM.
	     */
	    if (bktr->card.eepromAddr != 0) {

		u_char tuner_make;   /* Eg Philips, Temic */
		u_char tuner_tv_fm;  /* TV or TV with FM Radio */
		u_char tuner_format; /* Eg NTSC, PAL, SECAM */
		int    tuner;

		int tuner_0_table[] = {
			PHILIPS_NTSC,  PHILIPS_PAL,
			PHILIPS_PAL,   PHILIPS_PAL,
			PHILIPS_PAL,   PHILIPS_PAL,
			PHILIPS_SECAM, PHILIPS_SECAM,
			PHILIPS_SECAM, PHILIPS_PAL};

		int tuner_0_fm_table[] = {
			PHILIPS_FR1236_NTSC,  PHILIPS_FR1216_PAL,
			PHILIPS_FR1216_PAL,   PHILIPS_FR1216_PAL,
			PHILIPS_FR1216_PAL,   PHILIPS_FR1216_PAL,
			PHILIPS_FR1236_SECAM, PHILIPS_FR1236_SECAM,
			PHILIPS_FR1236_SECAM, PHILIPS_FR1216_PAL};

		int tuner_1_table[] = {
			TEMIC_NTSC,  TEMIC_PAL,   TEMIC_PAL,
			TEMIC_PAL,   TEMIC_PAL,   TEMIC_PAL,
			TEMIC_SECAM, TEMIC_SECAM, TEMIC_SECAM,
			TEMIC_PAL};


		/* Extract information from the EEPROM data */
	    	readEEProm(bktr, 0, 128, (u_char *) &eeprom );

		tuner_make   = (eeprom[0x41] & 0x7);
		tuner_tv_fm  = (eeprom[0x41] & 0x18) >> 3;
		tuner_format = (eeprom[0x42] & 0xf0) >> 4;

		/* Treat tuner make 0 (Philips) and make 2 (LG) the same */
		if ( ((tuner_make == 0) || (tuner_make == 2))
		    && (tuner_format <= 9) && (tuner_tv_fm == 0) ) {
			tuner = tuner_0_table[tuner_format];
			select_tuner( bktr, tuner );
			goto checkDBX;
		}

		if ( ((tuner_make == 0) || (tuner_make == 2))
		    && (tuner_format <= 9) && (tuner_tv_fm == 1) ) {
			tuner = tuner_0_fm_table[tuner_format];
			select_tuner( bktr, tuner );
			goto checkDBX;
		}

		if ( (tuner_make == 1) && (tuner_format <= 9) ) {
			tuner = tuner_1_table[tuner_format];
			select_tuner( bktr, tuner );
			goto checkDBX;
		}

	    	printf("Warning - Unknown AVerMedia Tuner Make %d Format %d\n",
			tuner_make, tuner_format);
	    }
	    break;

	} /* end switch(card) */


        /* At this point, a goto checkDBX has not occured */
        /* We have not been able to select a Tuner */
        /* Some cards make use of the tuner address to */
        /* identify the make/model of tuner */

        /* At address 0xc0/0xc1 we often find a TEMIC NTSC */
        if ( i2cRead( bktr, 0xc1 ) != ABSENT ) {
	    select_tuner( bktr, TEMIC_NTSC );
            goto checkDBX;
        }
  
        /* At address 0xc6/0xc7 we often find a PHILIPS NTSC Tuner */
        if ( i2cRead( bktr, 0xc7 ) != ABSENT ) {
	    select_tuner( bktr, PHILIPS_NTSC );
            goto checkDBX;
        }

        /* Address 0xc2/0xc3 is default (or common address) for several */
	/* tuners and we cannot tell which is which. */
	/* And for all other tuner i2c addresses, select the default */
	select_tuner( bktr, DEFAULT_TUNER );


checkDBX:
#if defined( OVERRIDE_DBX )
	bktr->card.dbx = OVERRIDE_DBX;
	goto checkMSP;
#endif
   /* Check for i2c devices */
	if (!any_i2c_devices) {
		goto checkMSP;
	}

	/* probe for BTSC (dbx) chip */
	if ( i2cRead( bktr, TDA9850_RADDR ) != ABSENT )
		bktr->card.dbx = 1;

checkMSP:
	/* If this is a Hauppauge Bt878 card, we need to enable the
	 * MSP 34xx audio chip. 
	 * If this is a Hauppauge Bt848 card, reset the MSP device.
	 * The MSP reset line is wired to GPIO pin 5. On Bt878 cards a pulldown
	 * resistor holds the device in reset until we set GPIO pin 5.
         */

	/* Optionally skip the MSP reset. This is handy if you initialise the
	 * MSP audio in another operating system (eg Windows) first and then
	 * do a soft reboot.
	 */

#ifndef BKTR_NO_MSP_RESET
	if (card == CARD_HAUPPAUGE) {
            bt848->gpio_out_en = bt848->gpio_out_en | (1<<5);
            bt848->gpio_data   = bt848->gpio_data | (1<<5);  /* write '1' */
            DELAY(2500); /* wait 2.5ms */
            bt848->gpio_data   = bt848->gpio_data & ~(1<<5); /* write '0' */
            DELAY(2500); /* wait 2.5ms */
            bt848->gpio_data   = bt848->gpio_data | (1<<5);  /* write '1' */
            DELAY(2500); /* wait 2.5ms */
        }
#endif

#if defined( OVERRIDE_MSP )
	bktr->card.msp3400c = OVERRIDE_MSP;
	goto checkMSPEnd;
#endif

	/* Check for i2c devices */
	if (!any_i2c_devices) {
		goto checkMSPEnd;
	}

	if ( i2cRead( bktr, MSP3400C_RADDR ) != ABSENT ) {
		bktr->card.msp3400c = 1;
	}

checkMSPEnd:

	if (bktr->card.msp3400c) {
		bktr->msp_addr = MSP3400C_WADDR;
		msp_read_id( bktr );
		printf("bktr%d: Detected a MSP%s at 0x%x\n", unit,
				bktr->msp_version_string,
				bktr->msp_addr);

	}


/* Start of Check Remote */
        /* Check for the Hauppauge IR Remote Control */
        /* If there is an external unit, the internal will be ignored */

        bktr->remote_control = 0; /* initial value */

        if (any_i2c_devices) {
            if (i2cRead( bktr, HAUP_REMOTE_EXT_RADDR ) != ABSENT )
                {
                bktr->remote_control      = 1;
                bktr->remote_control_addr = HAUP_REMOTE_EXT_RADDR;
                }
            else if (i2cRead( bktr, HAUP_REMOTE_INT_RADDR ) != ABSENT )
                {
                bktr->remote_control      = 1;
                bktr->remote_control_addr = HAUP_REMOTE_INT_RADDR;
                }

        }
        /* If a remote control is found, poll it 5 times to turn off the LED */
        if (bktr->remote_control) {
                int i;
                for (i=0; i<5; i++)
                        i2cRead( bktr, bktr->remote_control_addr );
        }
/* End of Check Remote */

#if defined( BKTR_USE_PLL )
	bktr->xtal_pll_mode = BT848_USE_PLL;
	goto checkPLLEnd;
#endif
	/* Default is to use XTALS and not PLL mode */
	bktr->xtal_pll_mode = BT848_USE_XTALS;

	/* Enable PLL mode for PAL/SECAM users on Hauppauge 878 cards */
	if ((card == CARD_HAUPPAUGE) &&
	   (bktr->id==BROOKTREE_878 || bktr->id==BROOKTREE_879) )
		bktr->xtal_pll_mode = BT848_USE_PLL;


	/* Enable PLL mode for OSPREY users */
	if (card == CARD_OSPREY)
		bktr->xtal_pll_mode = BT848_USE_PLL;


	/* Enable PLL mode for PAL/SECAM users on FlyVideo 878 cards */
	if ((card == CARD_FLYVIDEO) &&
	   (bktr->id==BROOKTREE_878 || bktr->id==BROOKTREE_879) )
		bktr->xtal_pll_mode = BT848_USE_PLL;


#if defined( BKTR_USE_PLL )
checkPLLEnd:
#endif


	bktr->card.tuner_pllAddr = tuner_i2c_address;

	if ( verbose ) {
		printf( "%s", bktr->card.name );
		if ( bktr->card.tuner )
			printf( ", %s tuner", bktr->card.tuner->name );
		if ( bktr->card.dbx )
			printf( ", dbx stereo" );
		if ( bktr->card.msp3400c )
			printf( ", msp3400c stereo" );
                if ( bktr->remote_control )
                        printf( ", remote control" );
		printf( ".\n" );
	}
}

#undef ABSENT

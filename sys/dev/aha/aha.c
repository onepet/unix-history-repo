/*
 * Generic register and struct definitions for the Adaptech 154x/164x
 * SCSI host adapters. Product specific probe and attach routines can
 * be found in:
 *      aha 1540/1542B/1542C/1542CF/1542CP	aha_isa.c
 *
 * Copyright (c) 1998 M. Warner Losh.
 * All Rights Reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Derived from bt.c written by:
 *
 * Copyright (c) 1998 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      $Id: aha.c,v 1.3 1998/09/17 00:08:29 gibbs Exp $
 */

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
 
#include <machine/bus_pio.h>
#include <machine/bus.h>
#include <machine/clock.h>

#include <cam/cam.h>
#include <cam/cam_ccb.h>
#include <cam/cam_sim.h>
#include <cam/cam_xpt_sim.h>
#include <cam/cam_debug.h>

#include <cam/scsi/scsi_message.h>

#include <vm/vm.h>
#include <vm/pmap.h>
 
#include <dev/aha/ahareg.h>

struct aha_softc *aha_softcs[NAHA];

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define	PRVERBOSE(x) if (bootverbose) printf x

/* Macro to determine that a rev is potentially a new valid one
 * so that the driver doesn't keep breaking on new revs as it
 * did for the CF and CP.
 */
#define PROBABLY_NEW_BOARD(REV) (REV > 0x43 && REV < 0x56)

/* MailBox Management functions */
static __inline void	ahanextinbox(struct aha_softc *aha);
static __inline void	ahanextoutbox(struct aha_softc *aha);

static __inline void
ahanextinbox(struct aha_softc *aha)
{
	if (aha->cur_inbox == aha->last_inbox)
		aha->cur_inbox = aha->in_boxes;
	else
		aha->cur_inbox++;
}

static __inline void
ahanextoutbox(struct aha_softc *aha)
{
	if (aha->cur_outbox == aha->last_outbox)
		aha->cur_outbox = aha->out_boxes;
	else
		aha->cur_outbox++;
}

#define ahautoa24(u,s3)			\
	(s3)[0] = ((u) >> 16) & 0xff;	\
	(s3)[1] = ((u) >> 8) & 0xff;	\
	(s3)[2] = (u) & 0xff;

#define aha_a24tou(s3) \
	(((s3)[0] << 16) | ((s3)[1] << 8) | (s3)[2])

/* CCB Mangement functions */
static __inline u_int32_t		ahaccbvtop(struct aha_softc *aha,
						  struct aha_ccb *bccb);
static __inline struct aha_ccb*		ahaccbptov(struct aha_softc *aha,
						  u_int32_t ccb_addr);

static __inline u_int32_t
ahaccbvtop(struct aha_softc *aha, struct aha_ccb *bccb)
{
	return (aha->aha_ccb_physbase
	      + (u_int32_t)((caddr_t)bccb - (caddr_t)aha->aha_ccb_array));
}
static __inline struct aha_ccb *
ahaccbptov(struct aha_softc *aha, u_int32_t ccb_addr)
{
	return (aha->aha_ccb_array +
	      + ((struct aha_ccb*)ccb_addr-(struct aha_ccb*)aha->aha_ccb_physbase));
}

static struct aha_ccb*	ahagetccb(struct aha_softc *aha);
static __inline void	ahafreeccb(struct aha_softc *aha, struct aha_ccb *bccb);
static void		ahaallocccbs(struct aha_softc *aha);
static bus_dmamap_callback_t ahaexecuteccb;
static void		ahadone(struct aha_softc *aha, struct aha_ccb *bccb,
			       aha_mbi_comp_code_t comp_code);

/* Host adapter command functions */
static int	ahareset(struct aha_softc* aha, int hard_reset);

/* Initialization functions */
static int			ahainitmboxes(struct aha_softc *aha);
static bus_dmamap_callback_t	ahamapmboxes;
static bus_dmamap_callback_t	ahamapccbs;
static bus_dmamap_callback_t	ahamapsgs;

/* Transfer Negotiation Functions */
static void ahafetchtransinfo(struct aha_softc *aha,
			     struct ccb_trans_settings *cts);

/* CAM SIM entry points */
#define ccb_bccb_ptr spriv_ptr0
#define ccb_aha_ptr spriv_ptr1
static void	ahaaction(struct cam_sim *sim, union ccb *ccb);
static void	ahapoll(struct cam_sim *sim);

/* Our timeout handler */
timeout_t ahatimeout;

u_long aha_unit = 0;

/*
 * Do our own re-probe protection until a configuration
 * manager can do it for us.  This ensures that we don't
 * reprobe a card already found by the EISA or PCI probes.
 */
struct aha_isa_port aha_isa_ports[] =
{
	{ 0x330, 0 },
	{ 0x334, 0 },
	{ 0x230, 0 },
	{ 0x234, 0 },
	{ 0x130, 0 },
	{ 0x134, 0 }
};

/* Exported functions */
struct aha_softc *
aha_alloc(int unit, bus_space_tag_t tag, bus_space_handle_t bsh)
{
	struct  aha_softc *aha;  
	int	i;

	if (unit != AHA_TEMP_UNIT) {
		if (unit >= NAHA) {
			printf("aha: unit number (%d) too high\n", unit);
			return NULL;
		}

		/*
		 * Allocate a storage area for us
		 */
		if (aha_softcs[unit]) {    
			printf("aha%d: memory already allocated\n", unit);
			return NULL;    
		}
	}

	aha = malloc(sizeof(struct aha_softc), M_DEVBUF, M_NOWAIT);
	if (!aha) {
		printf("aha%d: cannot malloc!\n", unit);
		return NULL;    
	}
	bzero(aha, sizeof(struct aha_softc));
	SLIST_INIT(&aha->free_aha_ccbs);
	LIST_INIT(&aha->pending_ccbs);
	SLIST_INIT(&aha->sg_maps);
	aha->unit = unit;
	aha->tag = tag;
	aha->bsh = bsh;

	if (aha->unit != AHA_TEMP_UNIT) {
		aha_softcs[unit] = aha;
	}
	return (aha);
}

void
aha_free(struct aha_softc *aha)
{
	switch (aha->init_level) {
	default:
	case 8:
	{
		struct sg_map_node *sg_map;

		while ((sg_map = SLIST_FIRST(&aha->sg_maps))!= NULL) {
			SLIST_REMOVE_HEAD(&aha->sg_maps, links);
			bus_dmamap_unload(aha->sg_dmat,
					  sg_map->sg_dmamap);
			bus_dmamem_free(aha->sg_dmat, sg_map->sg_vaddr,
					sg_map->sg_dmamap);
			free(sg_map, M_DEVBUF);
		}
		bus_dma_tag_destroy(aha->sg_dmat);
	}
	case 7:
		bus_dmamap_unload(aha->ccb_dmat, aha->ccb_dmamap);
	case 6:
		bus_dmamap_destroy(aha->ccb_dmat, aha->ccb_dmamap);
		bus_dmamem_free(aha->ccb_dmat, aha->aha_ccb_array,
				aha->ccb_dmamap);
	case 5:
		bus_dma_tag_destroy(aha->ccb_dmat);
	case 4:
		bus_dmamap_unload(aha->mailbox_dmat, aha->mailbox_dmamap);
	case 3:
		bus_dmamem_free(aha->mailbox_dmat, aha->in_boxes,
				aha->mailbox_dmamap);
		bus_dmamap_destroy(aha->mailbox_dmat, aha->mailbox_dmamap);
	case 2:
		bus_dma_tag_destroy(aha->buffer_dmat);
	case 1:
		bus_dma_tag_destroy(aha->mailbox_dmat);
	case 0:
	}
	if (aha->unit != AHA_TEMP_UNIT) {
		aha_softcs[aha->unit] = NULL;
	}
	free(aha, M_DEVBUF);
}

/*
 * Probe the adapter and verify that the card is an Adaptec.
 */
int
aha_probe(struct aha_softc* aha)
{
	u_int	 status;
	u_int	 intstat;
	int	 error;
	u_int8_t param;
	esetup_info_data_t esetup_info;

	/*
	 * See if the three I/O ports look reasonable.
	 * Touch the minimal number of registers in the
	 * failure case.
	 */
	status = aha_inb(aha, STATUS_REG);
	if ((status == 0)
	 || (status & (DIAG_ACTIVE|CMD_REG_BUSY|
		       STATUS_REG_RSVD|CMD_INVALID)) != 0) {
		return (ENXIO);
	}

	intstat = aha_inb(aha, INTSTAT_REG);
	if ((intstat & INTSTAT_REG_RSVD) != 0) {
		printf("%s: Failed Intstat Reg Test\n", aha_name(aha));
		return (ENXIO);
	}

	/*
	 * Looking good so far.  Final test is to reset the
	 * adapter.
	 */
	if ((error = ahareset(aha, /*hard_reset*/TRUE)) != 0) {
		if (bootverbose)
			printf("%s: Failed Reset\n", aha_name(aha));
		return (ENXIO);
	}
	
	/*
	 * Issue a buslogic command that will fail, and reject the board
	 * if it doesn't.
	 */
	param = sizeof(esetup_info);
	error = aha_cmd(aha, BOP_INQUIRE_ESETUP_INFO, &param, /*parmlen*/1,
		       (u_int8_t*)&esetup_info, sizeof(esetup_info),
		       DEFAULT_CMD_TIMEOUT);
	if (error == 0) {
		printf("%s: Almost: ESETUP failed\n", aha_name(aha));
		return ENXIO;
	}
	return (0);
}

/*
 * Pull the boards setup information and record it in our softc.
 */
int
aha_fetch_adapter_info(struct aha_softc *aha)
{
	board_id_data_t	board_id;
	setup_data_t	setup_info;
	config_data_t config_data;
	u_int8_t length_param;
	int	 error;
	struct	aha_extbios extbios;
	
	/* First record the firmware version */
	error = aha_cmd(aha, BOP_INQUIRE_BOARD_ID, NULL, /*parmlen*/0,
		       (u_int8_t*)&board_id, sizeof(board_id),
		       DEFAULT_CMD_TIMEOUT);
	if (error != 0) {
		if (bootverbose)
			printf("%s: INQUIRE failed %x\n", aha_name(aha), error);
		return (ENXIO);
	}
	aha->firmware_ver[0] = board_id.firmware_rev_major;
	aha->firmware_ver[1] = '.';
	aha->firmware_ver[2] = board_id.firmware_rev_minor;
	aha->firmware_ver[3] = '\0';

	aha->boardid = board_id.board_type;

	switch (board_id.board_type) {
	case BOARD_1540_16HEAD_BIOS:
		strcpy(aha->model, "1540 16 head BIOS");
		break;
	case BOARD_1540_64HEAD_BIOS:
		strcpy(aha->model, "1540 64 head BIOS");
		break;
	case BOARD_1542:
		strcpy(aha->model, "1540/1542 64 head BIOS");
		break;
	case BOARD_1640:
		strcpy(aha->model, "1640");
		break;
	case BOARD_1740:
		strcpy(aha->model, "1740A/1742A/1744");
		break;
	case BOARD_1542C:
		strcpy(aha->model, "1542C");
		break;
	case BOARD_1542CF:
		strcpy(aha->model, "1542CF");
		break;
	case BOARD_1542CP:
		strcpy(aha->model, "1542CP");
		break;
	default:
		strcpy(aha->model, "Unknown");
		break;
	}
	/*
	 * If we are a new type of 1542 board (anything newer than a 1542C)
	 * then disable the extended bios so that the
	 * mailbox interface is unlocked.
	 * This is also true for the 1542B Version 3.20. First Adaptec
	 * board that supports >1Gb drives.
	 * No need to check the extended bios flags as some of the
	 * extensions that cause us problems are not flagged in that byte.
	 */
	if (PROBABLY_NEW_BOARD(aha->boardid) ||
		(aha->boardid == 0x41
		&& board_id.firmware_rev_major == 0x31 && 
			board_id.firmware_rev_minor >= 0x34)) {
		error = aha_cmd(aha, BOP_RETURN_EXT_BIOS_INFO, NULL,
			/*paramlen*/0, (u_char *)&extbios, sizeof(extbios),
			DEFAULT_CMD_TIMEOUT);
		error = aha_cmd(aha, BOP_MBOX_IF_ENABLE, (u_int8_t *)&extbios,
			/*paramlen*/2, NULL, 0, DEFAULT_CMD_TIMEOUT);
	}
	if (aha->boardid < 0x41)
		printf("%s: Likely aha 1542A, which might not work properly\n",
			aha_name(aha));

	aha->max_sg = 17;
	aha->diff_bus = 0;
	aha->extended_lun = 0;
	aha->extended_trans = 0;
	aha->max_ccbs = 17;		/* Need 17 to do 64k I/O */
	/* Determine Sync/Wide/Disc settings */
	length_param = sizeof(setup_info);
	error = aha_cmd(aha, BOP_INQUIRE_SETUP_INFO, &length_param,
		       /*paramlen*/1, (u_int8_t*)&setup_info,
		       sizeof(setup_info), DEFAULT_CMD_TIMEOUT);
	if (error != 0) {
		printf("%s: aha_fetch_adapter_info - Failed "
		       "Get Setup Info\n", aha_name(aha));
		return (error);
	}
	if (setup_info.initiate_sync != 0) {
		aha->sync_permitted = ALL_TARGETS;
	}
	aha->disc_permitted = ALL_TARGETS;

	/* We need as many mailboxes as we can have ccbs */
	aha->num_boxes = aha->max_ccbs;

	/* Determine our SCSI ID */
	
	error = aha_cmd(aha, BOP_INQUIRE_CONFIG, NULL, /*parmlen*/0,
		       (u_int8_t*)&config_data, sizeof(config_data),
		       DEFAULT_CMD_TIMEOUT);
	if (error != 0) {
		printf("%s: aha_fetch_adapter_info - Failed Get Config\n",
		       aha_name(aha));
		return (error);
	}
	aha->scsi_id = config_data.scsi_id;
	return (0);
}

/*
 * Start the board, ready for normal operation
 */
int
aha_init(struct aha_softc* aha)
{
	/* Announce the Adapter */
	printf("%s: AHA-%s FW Rev. %s (ID=%x)", aha_name(aha),
	       aha->model, aha->firmware_ver, aha->boardid);

	if (aha->diff_bus != 0)
		printf("Diff ");

	printf("SCSI Host Adapter, SCSI ID %d, %d CCBs\n", aha->scsi_id,
	       aha->max_ccbs);

	/*
	 * Create our DMA tags.  These tags define the kinds of device
	 * accessable memory allocations and memory mappings we will 
	 * need to perform during normal operation.
	 *
	 * Unless we need to further restrict the allocation, we rely
	 * on the restrictions of the parent dmat, hence the common
	 * use of MAXADDR and MAXSIZE.
	 */

	/* DMA tag for mapping buffers into device visible space. */
	if (bus_dma_tag_create(aha->parent_dmat, /*alignment*/0, /*boundary*/0,
			       /*lowaddr*/BUS_SPACE_MAXADDR,
			       /*highaddr*/BUS_SPACE_MAXADDR,
			       /*filter*/NULL, /*filterarg*/NULL,
			       /*maxsize*/MAXBSIZE, /*nsegments*/AHA_NSEG,
			       /*maxsegsz*/BUS_SPACE_MAXSIZE_24BIT,
			       /*flags*/BUS_DMA_ALLOCNOW,
			       &aha->buffer_dmat) != 0) {
		goto error_exit;
	}

	aha->init_level++;
	/* DMA tag for our mailboxes */
	if (bus_dma_tag_create(aha->parent_dmat, /*alignment*/0, /*boundary*/0,
			       /*lowaddr*/BUS_SPACE_MAXADDR,
			       /*highaddr*/BUS_SPACE_MAXADDR,
			       /*filter*/NULL, /*filterarg*/NULL,
			       aha->num_boxes * (sizeof(aha_mbox_in_t)
					       + sizeof(aha_mbox_out_t)),
			       /*nsegments*/1,
			       /*maxsegsz*/BUS_SPACE_MAXSIZE_24BIT,
			       /*flags*/0, &aha->mailbox_dmat) != 0) {
		goto error_exit;
        }

	aha->init_level++;

	/* Allocation for our mailboxes */
	if (bus_dmamem_alloc(aha->mailbox_dmat, (void **)&aha->out_boxes,
			     BUS_DMA_NOWAIT, &aha->mailbox_dmamap) != 0) {
		goto error_exit;
	}

	aha->init_level++;

	/* And permanently map them */
	bus_dmamap_load(aha->mailbox_dmat, aha->mailbox_dmamap,
       			aha->out_boxes,
			aha->num_boxes * (sizeof(aha_mbox_in_t)
				       + sizeof(aha_mbox_out_t)),
			ahamapmboxes, aha, /*flags*/0);

	aha->init_level++;

	aha->in_boxes = (aha_mbox_in_t *)&aha->out_boxes[aha->num_boxes];

	ahainitmboxes(aha);

	/* DMA tag for our ccb structures */
	if (bus_dma_tag_create(aha->parent_dmat, /*alignment*/0, /*boundary*/0,
			       /*lowaddr*/BUS_SPACE_MAXADDR,
			       /*highaddr*/BUS_SPACE_MAXADDR,
			       /*filter*/NULL, /*filterarg*/NULL,
			       aha->max_ccbs * sizeof(struct aha_ccb),
			       /*nsegments*/1,
			       /*maxsegsz*/BUS_SPACE_MAXSIZE_24BIT,
			       /*flags*/0, &aha->ccb_dmat) != 0) {
		goto error_exit;
        }

	aha->init_level++;

	/* Allocation for our ccbs */
	if (bus_dmamem_alloc(aha->ccb_dmat, (void **)&aha->aha_ccb_array,
			     BUS_DMA_NOWAIT, &aha->ccb_dmamap) != 0) {
		goto error_exit;
	}

	aha->init_level++;

	/* And permanently map them */
	bus_dmamap_load(aha->ccb_dmat, aha->ccb_dmamap,
       			aha->aha_ccb_array,
			aha->max_ccbs * sizeof(struct aha_ccb),
			ahamapccbs, aha, /*flags*/0);

	aha->init_level++;

	/* DMA tag for our S/G structures.  We allocate in page sized chunks */
	if (bus_dma_tag_create(aha->parent_dmat, /*alignment*/0, /*boundary*/0,
			       /*lowaddr*/BUS_SPACE_MAXADDR,
			       /*highaddr*/BUS_SPACE_MAXADDR,
			       /*filter*/NULL, /*filterarg*/NULL,
			       PAGE_SIZE, /*nsegments*/1,
			       /*maxsegsz*/BUS_SPACE_MAXSIZE_24BIT,
			       /*flags*/0, &aha->sg_dmat) != 0) {
		goto error_exit;
        }

	aha->init_level++;

	/* Perform initial CCB allocation */
	bzero(aha->aha_ccb_array, aha->max_ccbs * sizeof(struct aha_ccb));
	ahaallocccbs(aha);

	if (aha->num_ccbs == 0) {
		printf("%s: aha_init - Unable to allocate initial ccbs\n",
		       aha_name(aha));
		goto error_exit;
	}

	/*
	 * Note that we are going and return (to probe)
	 */
	return 0;

error_exit:

	return (ENXIO);
}

int
aha_attach(struct aha_softc *aha)
{
	int tagged_dev_openings;
	struct cam_devq *devq;

	/*
	 * We reserve 1 ccb for error recovery, so don't
	 * tell the XPT about it.
	 */
	tagged_dev_openings = 0;

	/*
	 * Create the device queue for our SIM.
	 */
	devq = cam_simq_alloc(aha->max_ccbs - 1);
	if (devq == NULL)
		return (ENOMEM);

	/*
	 * Construct our SIM entry
	 */
	aha->sim = cam_sim_alloc(ahaaction, ahapoll, "aha", aha, aha->unit,
				2, tagged_dev_openings, devq);
	if (aha->sim == NULL) {
		cam_simq_free(devq);
		return (ENOMEM);
	}
	
	if (xpt_bus_register(aha->sim, 0) != CAM_SUCCESS) {
		cam_sim_free(aha->sim, /*free_devq*/TRUE);
		return (ENXIO);
	}
	
	if (xpt_create_path(&aha->path, /*periph*/NULL,
			    cam_sim_path(aha->sim), CAM_TARGET_WILDCARD,
			    CAM_LUN_WILDCARD) != CAM_REQ_CMP) {
		xpt_bus_deregister(cam_sim_path(aha->sim));
		cam_sim_free(aha->sim, /*free_devq*/TRUE);
		return (ENXIO);
	}
		
	return (0);
}

char *
aha_name(struct aha_softc *aha)
{
	static char name[10];

	sprintf(name, "aha%d", aha->unit);
	return (name);
}

int
aha_check_probed_iop(u_int ioport)
{
	u_int i;

	for (i=0; i < AHA_NUM_ISAPORTS; i++) {
		if (aha_isa_ports[i].addr == ioport) {
			if (aha_isa_ports[i].probed != 0)
				return (1);
			else {
				return (0);
			}
		}
	}
	return (1);
}

void
aha_mark_probed_bio(isa_compat_io_t port)
{
	if (port < BIO_DISABLED)
		aha_isa_ports[port].probed = 1;
}

void
aha_mark_probed_iop(u_int ioport)
{
	u_int i;

	for (i = 0; i < AHA_NUM_ISAPORTS; i++) {
		if (ioport == aha_isa_ports[i].addr) {
			aha_isa_ports[i].probed = 1;
			break;
		}
	}
}

static void
ahaallocccbs(struct aha_softc *aha)
{
	struct aha_ccb *next_ccb;
	struct sg_map_node *sg_map;
	bus_addr_t physaddr;
	aha_sg_t *segs;
	int newcount;
	int i;

	next_ccb = &aha->aha_ccb_array[aha->num_ccbs];

	sg_map = malloc(sizeof(*sg_map), M_DEVBUF, M_NOWAIT);

	if (sg_map == NULL)
		return;

	/* Allocate S/G space for the next batch of CCBS */
	if (bus_dmamem_alloc(aha->sg_dmat, (void **)&sg_map->sg_vaddr,
			     BUS_DMA_NOWAIT, &sg_map->sg_dmamap) != 0) {
		free(sg_map, M_DEVBUF);
		return;
	}

	SLIST_INSERT_HEAD(&aha->sg_maps, sg_map, links);

	bus_dmamap_load(aha->sg_dmat, sg_map->sg_dmamap, sg_map->sg_vaddr,
			PAGE_SIZE, ahamapsgs, aha, /*flags*/0);
	
	segs = sg_map->sg_vaddr;
	physaddr = sg_map->sg_physaddr;

	newcount = (PAGE_SIZE / (AHA_NSEG * sizeof(aha_sg_t)));
	for (i = 0; aha->num_ccbs < aha->max_ccbs && i < newcount; i++) {
		int error;

		next_ccb->sg_list = segs;
		next_ccb->sg_list_phys = physaddr;
		next_ccb->flags = BCCB_FREE;
		error = bus_dmamap_create(aha->buffer_dmat, /*flags*/0,
					  &next_ccb->dmamap);
		if (error != 0)
			break;
		SLIST_INSERT_HEAD(&aha->free_aha_ccbs, next_ccb, links);
		segs += AHA_NSEG;
		physaddr += (AHA_NSEG * sizeof(aha_sg_t));
		next_ccb++;
		aha->num_ccbs++;
	}

	/* Reserve a CCB for error recovery */
	if (aha->recovery_bccb == NULL) {
		aha->recovery_bccb = SLIST_FIRST(&aha->free_aha_ccbs);
		SLIST_REMOVE_HEAD(&aha->free_aha_ccbs, links);
	}
}

static __inline void
ahafreeccb(struct aha_softc *aha, struct aha_ccb *bccb)
{
	int s;

	s = splcam();
	if ((bccb->flags & BCCB_ACTIVE) != 0)
		LIST_REMOVE(&bccb->ccb->ccb_h, sim_links.le);
	if (aha->resource_shortage != 0
	 && (bccb->ccb->ccb_h.status & CAM_RELEASE_SIMQ) == 0) {
		bccb->ccb->ccb_h.status |= CAM_RELEASE_SIMQ;
		aha->resource_shortage = FALSE;
	}
	bccb->flags = BCCB_FREE;
	SLIST_INSERT_HEAD(&aha->free_aha_ccbs, bccb, links);
	splx(s);
}

static struct aha_ccb*
ahagetccb(struct aha_softc *aha)
{
	struct	aha_ccb* bccb;
	int	s;

	s = splcam();
	if ((bccb = SLIST_FIRST(&aha->free_aha_ccbs)) != NULL) {
		SLIST_REMOVE_HEAD(&aha->free_aha_ccbs, links);
	} else if (aha->num_ccbs < aha->max_ccbs) {
		ahaallocccbs(aha);
		bccb = SLIST_FIRST(&aha->free_aha_ccbs);
		if (bccb == NULL)
			printf("%s: Can't malloc BCCB\n", aha_name(aha));
		else
			SLIST_REMOVE_HEAD(&aha->free_aha_ccbs, links);
	}
	splx(s);

	return (bccb);
}

static void
ahaaction(struct cam_sim *sim, union ccb *ccb)
{
	struct	aha_softc *aha;
	int	s;

	CAM_DEBUG(ccb->ccb_h.path, CAM_DEBUG_TRACE, ("ahaaction\n"));
	
	aha = (struct aha_softc *)cam_sim_softc(sim);
	
	switch (ccb->ccb_h.func_code) {
	/* Common cases first */
	case XPT_SCSI_IO:	/* Execute the requested I/O operation */
	case XPT_RESET_DEV:	/* Bus Device Reset the specified SCSI device */
	{
		struct	aha_ccb	*bccb;
		struct	aha_hccb *hccb;

		/*
		 * get a bccb to use.
		 */
		if ((bccb = ahagetccb(aha)) == NULL) {
			int s;

			s = splcam();
			aha->resource_shortage = TRUE;
			splx(s);
			xpt_freeze_simq(aha->sim, /*count*/1);
			ccb->ccb_h.status = CAM_REQUEUE_REQ;
			xpt_done(ccb);
			return;
		}
		
		hccb = &bccb->hccb;

		/*
		 * So we can find the BCCB when an abort is requested
		 */
		bccb->ccb = ccb;
		ccb->ccb_h.ccb_bccb_ptr = bccb;
		ccb->ccb_h.ccb_aha_ptr = aha;

		/*
		 * Put all the arguments for the xfer in the bccb
		 */
		hccb->target = ccb->ccb_h.target_id;
		hccb->lun = ccb->ccb_h.target_lun;
		hccb->ahastat = 0;
		hccb->sdstat = 0;

		if (ccb->ccb_h.func_code == XPT_SCSI_IO) {
			struct ccb_scsiio *csio;
			struct ccb_hdr *ccbh;

			csio = &ccb->csio;
			ccbh = &csio->ccb_h;
			hccb->opcode = INITIATOR_CCB_WRESID;
			hccb->datain = (ccb->ccb_h.flags & CAM_DIR_IN) != 0;
			hccb->dataout = (ccb->ccb_h.flags & CAM_DIR_OUT) != 0;
			hccb->cmd_len = csio->cdb_len;
			if (hccb->cmd_len > sizeof(hccb->scsi_cdb)) {
				ccb->ccb_h.status = CAM_REQ_INVALID;
				ahafreeccb(aha, bccb);
				xpt_done(ccb);
				return;
			}
			hccb->sense_len = csio->sense_len;
			if ((ccbh->flags & CAM_CDB_POINTER) != 0) {
				if ((ccbh->flags & CAM_CDB_PHYS) == 0) {
					bcopy(csio->cdb_io.cdb_ptr,
					      hccb->scsi_cdb, hccb->cmd_len);
				} else {
					/* I guess I could map it in... */
					ccbh->status = CAM_REQ_INVALID;
					ahafreeccb(aha, bccb);
					xpt_done(ccb);
					return;
				}
			} else {
				bcopy(csio->cdb_io.cdb_bytes,
				      hccb->scsi_cdb, hccb->cmd_len);
			}
			/*
			 * If we have any data to send with this command,
			 * map it into bus space.
			 */
		        /* Only use S/G if there is a transfer */
			if ((ccbh->flags & CAM_DIR_MASK) != CAM_DIR_NONE) {
				if ((ccbh->flags & CAM_SCATTER_VALID) == 0) {
					/*
					 * We've been given a pointer
					 * to a single buffer.
					 */
					if ((ccbh->flags & CAM_DATA_PHYS)==0) {
						int s;
						int error;

						s = splsoftvm();
						error = bus_dmamap_load(
						    aha->buffer_dmat,
						    bccb->dmamap,
						    csio->data_ptr,
						    csio->dxfer_len,
						    ahaexecuteccb,
						    bccb,
						    /*flags*/0);
						if (error == EINPROGRESS) {
							/*
							 * So as to maintain
							 * ordering, freeze the
							 * controller queue
							 * until our mapping is
							 * returned.
							 */
							xpt_freeze_simq(aha->sim,
									1);
							csio->ccb_h.status |=
							    CAM_RELEASE_SIMQ;
						}
						splx(s);
					} else {
						struct bus_dma_segment seg; 

						/* Pointer to physical buffer */
						seg.ds_addr =
						    (bus_addr_t)csio->data_ptr;
						seg.ds_len = csio->dxfer_len;
						ahaexecuteccb(bccb, &seg, 1, 0);
					}
				} else {
					struct bus_dma_segment *segs;

					if ((ccbh->flags & CAM_DATA_PHYS) != 0)
						panic("ahaaction - Physical "
						      "segment pointers "
						      "unsupported");

					if ((ccbh->flags&CAM_SG_LIST_PHYS)==0)
						panic("ahaaction - Virtual "
						      "segment addresses "
						      "unsupported");

					/* Just use the segments provided */
					segs = (struct bus_dma_segment *)
					    csio->data_ptr;
					ahaexecuteccb(bccb, segs,
						     csio->sglist_cnt, 0);
				}
			} else {
				ahaexecuteccb(bccb, NULL, 0, 0);
			}
		} else {
			hccb->opcode = INITIATOR_BUS_DEV_RESET;
			/* No data transfer */
			hccb->datain = TRUE;
			hccb->dataout = TRUE;
			hccb->cmd_len = 0;
			hccb->sense_len = 0;
			ahaexecuteccb(bccb, NULL, 0, 0);
		}
		break;
	}
	case XPT_EN_LUN:		/* Enable LUN as a target */
	case XPT_TARGET_IO:		/* Execute target I/O request */
	case XPT_ACCEPT_TARGET_IO:	/* Accept Host Target Mode CDB */
	case XPT_CONT_TARGET_IO:	/* Continue Host Target I/O Connection*/
	case XPT_ABORT:			/* Abort the specified CCB */
		/* XXX Implement */
		ccb->ccb_h.status = CAM_REQ_INVALID;
		xpt_done(ccb);
		break;
	case XPT_SET_TRAN_SETTINGS:
	{
		/* XXX Implement */
		ccb->ccb_h.status = CAM_REQ_CMP;
		xpt_done(ccb);
		break;
	}
	case XPT_GET_TRAN_SETTINGS:
	/* Get default/user set transfer settings for the target */
	{
		struct	ccb_trans_settings *cts;
		u_int	target_mask;

		cts = &ccb->cts;
		target_mask = 0x01 << ccb->ccb_h.target_id;
		if ((cts->flags & CCB_TRANS_USER_SETTINGS) != 0) {
			cts->flags = 0;
			if ((aha->disc_permitted & target_mask) != 0)
				cts->flags |= CCB_TRANS_DISC_ENB;
			cts->bus_width = MSG_EXT_WDTR_BUS_8_BIT;
			if ((aha->sync_permitted & target_mask) != 0)
				cts->sync_period = 50;
			else
				cts->sync_period = 0;

			if (cts->sync_period != 0)
				cts->sync_offset = 15;

			cts->valid = CCB_TRANS_SYNC_RATE_VALID
				   | CCB_TRANS_SYNC_OFFSET_VALID
				   | CCB_TRANS_BUS_WIDTH_VALID
				   | CCB_TRANS_DISC_VALID
				   | CCB_TRANS_TQ_VALID;
		} else {
			ahafetchtransinfo(aha, cts);
		}

		ccb->ccb_h.status = CAM_REQ_CMP;
		xpt_done(ccb);
		break;
	}
	case XPT_CALC_GEOMETRY:
	{
		struct	  ccb_calc_geometry *ccg;
		u_int32_t size_mb;
		u_int32_t secs_per_cylinder;

		ccg = &ccb->ccg;
		size_mb = ccg->volume_size
			/ ((1024L * 1024L) / ccg->block_size);
		
		if (size_mb >= 1024 && (aha->extended_trans != 0)) {
			if (size_mb >= 2048) {
				ccg->heads = 255;
				ccg->secs_per_track = 63;
			} else {
				ccg->heads = 128;
				ccg->secs_per_track = 32;
			}
		} else {
			ccg->heads = 64;
			ccg->secs_per_track = 32;
		}
		secs_per_cylinder = ccg->heads * ccg->secs_per_track;
		ccg->cylinders = ccg->volume_size / secs_per_cylinder;
		ccb->ccb_h.status = CAM_REQ_CMP;
		xpt_done(ccb);
		break;
	}
	case XPT_RESET_BUS:		/* Reset the specified SCSI bus */
	{
		ahareset(aha, /*hardreset*/TRUE);
		ccb->ccb_h.status = CAM_REQ_CMP;
		xpt_done(ccb);
		break;
	}
	case XPT_TERM_IO:		/* Terminate the I/O process */
		/* XXX Implement */
		ccb->ccb_h.status = CAM_REQ_INVALID;
		xpt_done(ccb);
		break;
	case XPT_PATH_INQ:		/* Path routing inquiry */
	{
		struct ccb_pathinq *cpi = &ccb->cpi;
		
		cpi->version_num = 1; /* XXX??? */
		cpi->hba_inquiry = PI_SDTR_ABLE;
		cpi->target_sprt = 0;
		cpi->hba_misc = 0;
		cpi->hba_eng_cnt = 0;
		cpi->max_target = aha->wide_bus ? 15 : 7;
		cpi->max_lun = 7;
		cpi->initiator_id = aha->scsi_id;
		cpi->bus_id = cam_sim_bus(sim);
		strncpy(cpi->sim_vid, "FreeBSD", SIM_IDLEN);
		strncpy(cpi->hba_vid, "Adaptec", HBA_IDLEN);
		strncpy(cpi->dev_name, cam_sim_name(sim), DEV_IDLEN);
		cpi->unit_number = cam_sim_unit(sim);
		cpi->ccb_h.status = CAM_REQ_CMP;
		xpt_done(ccb);
		break;
	}
	default:
		ccb->ccb_h.status = CAM_REQ_INVALID;
		xpt_done(ccb);
		break;
	}
}

static void
ahaexecuteccb(void *arg, bus_dma_segment_t *dm_segs, int nseg, int error)
{
	struct	 aha_ccb *bccb;
	union	 ccb *ccb;
	struct	 aha_softc *aha;
	int	 s, i;
	u_int32_t paddr;

	bccb = (struct aha_ccb *)arg;
	ccb = bccb->ccb;
	aha = (struct aha_softc *)ccb->ccb_h.ccb_aha_ptr;

	if (error != 0) {
		if (error != EFBIG)
			printf("%s: Unexepected error 0x%x returned from "
			       "bus_dmamap_load\n", aha_name(aha), error);
		if (ccb->ccb_h.status == CAM_REQ_INPROG) {
			xpt_freeze_devq(ccb->ccb_h.path, /*count*/1);
			ccb->ccb_h.status = CAM_REQ_TOO_BIG|CAM_DEV_QFRZN;
		}
		ahafreeccb(aha, bccb);
		xpt_done(ccb);
		return;
	}
		
	if (nseg != 0) {
		aha_sg_t *sg;
		bus_dma_segment_t *end_seg;
		bus_dmasync_op_t op;

		end_seg = dm_segs + nseg;

		/* Copy the segments into our SG list */
		sg = bccb->sg_list;
		while (dm_segs < end_seg) {
			ahautoa24(dm_segs->ds_len, sg->len);
			ahautoa24(dm_segs->ds_addr, sg->addr);
			sg++;
			dm_segs++;
		}

		if (nseg > 1) {
			bccb->hccb.opcode = INITIATOR_SG_CCB_WRESID;
			ahautoa24((sizeof(aha_sg_t) * nseg),
				  bccb->hccb.data_len);
			ahautoa24(bccb->sg_list_phys, bccb->hccb.data_addr);
		} else {
			bcopy(bccb->sg_list->len, bccb->hccb.data_len, 3);
			bcopy(bccb->sg_list->addr, bccb->hccb.data_addr, 3);
		}

		if ((ccb->ccb_h.flags & CAM_DIR_MASK) == CAM_DIR_IN)
			op = BUS_DMASYNC_PREREAD;
		else
			op = BUS_DMASYNC_PREWRITE;

		bus_dmamap_sync(aha->buffer_dmat, bccb->dmamap, op);

	} else {
		bccb->hccb.opcode = INITIATOR_CCB_WRESID;
		ahautoa24(0, bccb->hccb.data_len);
		ahautoa24(0, bccb->hccb.data_addr);
	}

	s = splcam();

	/*
	 * Last time we need to check if this CCB needs to
	 * be aborted.
	 */
	if (ccb->ccb_h.status != CAM_REQ_INPROG) {
		if (nseg != 0)
			bus_dmamap_unload(aha->buffer_dmat, bccb->dmamap);
		ahafreeccb(aha, bccb);
		xpt_done(ccb);
		splx(s);
		return;
	}
		
	bccb->flags = BCCB_ACTIVE;
	ccb->ccb_h.status |= CAM_SIM_QUEUED;
	LIST_INSERT_HEAD(&aha->pending_ccbs, &ccb->ccb_h, sim_links.le);

	ccb->ccb_h.timeout_ch =
	    timeout(ahatimeout, (caddr_t)bccb,
		    (ccb->ccb_h.timeout * hz) / 1000);

	/* Tell the adapter about this command */
	paddr = ahaccbvtop(aha, bccb);
	ahautoa24(paddr, aha->cur_outbox->ccb_addr);
	if (aha->cur_outbox->action_code != BMBO_FREE)
		panic("%s: Too few mailboxes or to many ccbs???", aha_name(aha));
	aha->cur_outbox->action_code = BMBO_START;	
	aha_outb(aha, COMMAND_REG, BOP_START_MBOX);

	ahanextoutbox(aha);
	splx(s);
}

void
aha_intr(void *arg)
{
	struct	aha_softc *aha;
	u_int	intstat;

	aha = (struct aha_softc *)arg;
	while (((intstat = aha_inb(aha, INTSTAT_REG)) & INTR_PENDING) != 0) {
		if ((intstat & CMD_COMPLETE) != 0) {
			aha->latched_status = aha_inb(aha, STATUS_REG);
			aha->command_cmp = TRUE;
		}

		aha_outb(aha, CONTROL_REG, RESET_INTR);

		if ((intstat & IMB_LOADED) != 0) {
			while (aha->cur_inbox->comp_code != BMBI_FREE) {
				u_int32_t	paddr;
				paddr = aha_a24tou(aha->cur_inbox->ccb_addr);
				ahadone(aha,
				       ahaccbptov(aha, paddr),
				       aha->cur_inbox->comp_code);
				aha->cur_inbox->comp_code = BMBI_FREE;
				ahanextinbox(aha);
			}
		}

		if ((intstat & SCSI_BUS_RESET) != 0) {
			ahareset(aha, /*hardreset*/FALSE);
		}
	}
}

static void
ahadone(struct aha_softc *aha, struct aha_ccb *bccb, aha_mbi_comp_code_t comp_code)
{
	union  ccb	  *ccb;
	struct ccb_scsiio *csio;

	ccb = bccb->ccb;
	csio = &bccb->ccb->csio;

	if ((bccb->flags & BCCB_ACTIVE) == 0) {
		printf("%s: ahadone - Attempt to free non-active BCCB %p\n",
		       aha_name(aha), (void *)bccb);
		return;
	}

	if ((ccb->ccb_h.flags & CAM_DIR_MASK) != CAM_DIR_NONE) {
		bus_dmasync_op_t op;

		if ((ccb->ccb_h.flags & CAM_DIR_MASK) == CAM_DIR_IN)
			op = BUS_DMASYNC_POSTREAD;
		else
			op = BUS_DMASYNC_POSTWRITE;
		bus_dmamap_sync(aha->buffer_dmat, bccb->dmamap, op);
		bus_dmamap_unload(aha->buffer_dmat, bccb->dmamap);
	}

	if (bccb == aha->recovery_bccb) {
		/*
		 * The recovery BCCB does not have a CCB associated
		 * with it, so short circuit the normal error handling.
		 * We now traverse our list of pending CCBs and process
		 * any that were terminated by the recovery CCBs action.
		 * We also reinstate timeouts for all remaining, pending,
		 * CCBs.
		 */
		struct cam_path *path;
		struct ccb_hdr *ccb_h;
		cam_status error;

		/* Notify all clients that a BDR occured */
		error = xpt_create_path(&path, /*periph*/NULL,
					cam_sim_path(aha->sim),
					bccb->hccb.target,
					CAM_LUN_WILDCARD);
		
		if (error == CAM_REQ_CMP)
			xpt_async(AC_SENT_BDR, path, NULL);

		ccb_h = LIST_FIRST(&aha->pending_ccbs);
		while (ccb_h != NULL) {
			struct aha_ccb *pending_bccb;

			pending_bccb = (struct aha_ccb *)ccb_h->ccb_bccb_ptr;
			if (pending_bccb->hccb.target == bccb->hccb.target) {
				pending_bccb->hccb.ahastat = AHASTAT_HA_BDR;
				ccb_h = LIST_NEXT(ccb_h, sim_links.le);
				ahadone(aha, pending_bccb, BMBI_ERROR);
			} else {
				ccb_h->timeout_ch =
				    timeout(ahatimeout, (caddr_t)pending_bccb,
					    (ccb_h->timeout * hz) / 1000);
				ccb_h = LIST_NEXT(ccb_h, sim_links.le);
			}
		}
		printf("%s: No longer in timeout\n", aha_name(aha));
		return;
	}

	untimeout(ahatimeout, bccb, ccb->ccb_h.timeout_ch);

	switch (comp_code) {
	case BMBI_FREE:
		printf("%s: ahadone - CCB completed with free status!\n",
		       aha_name(aha));
		break;
	case BMBI_NOT_FOUND:
		printf("%s: ahadone - CCB Abort failed to find CCB\n",
		       aha_name(aha));
		break;
	case BMBI_ABORT:
	case BMBI_ERROR:
		/* An error occured */
		switch(bccb->hccb.ahastat) {
		case AHASTAT_DATARUN_ERROR:
			if (bccb->hccb.data_len <= 0) {
				csio->ccb_h.status = CAM_DATA_RUN_ERR;
				break;
			}
			/* FALLTHROUGH */
		case AHASTAT_NOERROR:
			csio->scsi_status = bccb->hccb.sdstat;
			csio->ccb_h.status |= CAM_SCSI_STATUS_ERROR;
			switch(csio->scsi_status) {
			case SCSI_STATUS_CHECK_COND:
			case SCSI_STATUS_CMD_TERMINATED:
				csio->ccb_h.status |= CAM_AUTOSNS_VALID;
				/*
				 * The aha writes the sense data at different
				 * offsets based on the scsi cmd len
				 */
				bcopy((caddr_t) &bccb->hccb.scsi_cdb +
					bccb->hccb.cmd_len, 
					(caddr_t) &csio->sense_data,
					bccb->hccb.sense_len);
				break;
			default:
				break;
			case SCSI_STATUS_OK:
				csio->ccb_h.status = CAM_REQ_CMP;
				break;
			}
			csio->resid = aha_a24tou(bccb->hccb.data_len);
			break;
		case AHASTAT_SELTIMEOUT:
			csio->ccb_h.status = CAM_SEL_TIMEOUT;
			break;
		case AHASTAT_UNEXPECTED_BUSFREE:
			csio->ccb_h.status = CAM_UNEXP_BUSFREE;
			break;
		case AHASTAT_INVALID_PHASE:
			csio->ccb_h.status = CAM_SEQUENCE_FAIL;
			break;
		case AHASTAT_INVALID_ACTION_CODE:
			panic("%s: Inavlid Action code", aha_name(aha));
			break;
		case AHASTAT_INVALID_OPCODE:
			panic("%s: Inavlid CCB Opcode code %x hccb = %p",
			      aha_name(aha), bccb->hccb.opcode, &bccb->hccb);
			break;
		case AHASTAT_LINKED_CCB_LUN_MISMATCH:
			/* We don't even support linked commands... */
			panic("%s: Linked CCB Lun Mismatch", aha_name(aha));
			break;
		case AHASTAT_INVALID_CCB_OR_SG_PARAM:
			panic("%s: Invalid CCB or SG list", aha_name(aha));
			break;
		case AHASTAT_HA_SCSI_BUS_RESET:
			if ((csio->ccb_h.status & CAM_STATUS_MASK)
			 != CAM_CMD_TIMEOUT)
				csio->ccb_h.status = CAM_SCSI_BUS_RESET;
			break;
		case AHASTAT_HA_BDR:
			if ((bccb->flags & BCCB_DEVICE_RESET) == 0)
				csio->ccb_h.status = CAM_BDR_SENT;
			else
				csio->ccb_h.status = CAM_CMD_TIMEOUT;
			break;
		}
		if (csio->ccb_h.status != CAM_REQ_CMP) {
			xpt_freeze_devq(csio->ccb_h.path, /*count*/1);
			csio->ccb_h.status |= CAM_DEV_QFRZN;
		}
		if ((bccb->flags & BCCB_RELEASE_SIMQ) != 0)
			ccb->ccb_h.status |= CAM_RELEASE_SIMQ;
		ahafreeccb(aha, bccb);
		xpt_done(ccb);
		break;
	case BMBI_OK:
		/* All completed without incident */
		/* XXX DO WE NEED TO COPY SENSE BYTES HERE???? XXX */
		ccb->ccb_h.status |= CAM_REQ_CMP;
		if ((bccb->flags & BCCB_RELEASE_SIMQ) != 0)
			ccb->ccb_h.status |= CAM_RELEASE_SIMQ;
		ahafreeccb(aha, bccb);
		xpt_done(ccb);
		break;
	}
}

static int
ahareset(struct aha_softc* aha, int hard_reset)
{
	struct	 ccb_hdr *ccb_h;
	u_int	 status;
	u_int	 timeout;
	u_int8_t reset_type;

	if (hard_reset != 0)
		reset_type = HARD_RESET;
	else
		reset_type = SOFT_RESET;
	aha_outb(aha, CONTROL_REG, reset_type);

	/* Wait 5sec. for Diagnostic start */
	timeout = 5 * 10000;
	while (--timeout) {
		status = aha_inb(aha, STATUS_REG);
		if ((status & DIAG_ACTIVE) != 0)
			break;
		DELAY(100);
	}
	if (timeout == 0) {
		if (bootverbose)
			printf("%s: ahareset - Diagnostic Active failed to "
				"assert. status = 0x%x\n", aha_name(aha),
				status);
		return (ETIMEDOUT);
	}

	/* Wait 10sec. for Diagnostic end */
	timeout = 10 * 10000;
	while (--timeout) {
		status = aha_inb(aha, STATUS_REG);
		if ((status & DIAG_ACTIVE) == 0)
			break;
		DELAY(100);
	}
	if (timeout == 0) {
		panic("%s: ahareset - Diagnostic Active failed to drop. "
		       "status = 0x%x\n", aha_name(aha), status);
		return (ETIMEDOUT);
	}

	/* Wait for the host adapter to become ready or report a failure */
	timeout = 10000;
	while (--timeout) {
		status = aha_inb(aha, STATUS_REG);
		if ((status & (DIAG_FAIL|HA_READY|DATAIN_REG_READY)) != 0)
			break;
		DELAY(100);
	}
	if (timeout == 0) {
		printf("%s: ahareset - Host adapter failed to come ready. "
		       "status = 0x%x\n", aha_name(aha), status);
		return (ETIMEDOUT);
	}

	/* If the diagnostics failed, tell the user */
	if ((status & DIAG_FAIL) != 0
	 || (status & HA_READY) == 0) {
		printf("%s: ahareset - Adapter failed diagnostics\n",
		       aha_name(aha));

		if ((status & DATAIN_REG_READY) != 0)
			printf("%s: ahareset - Host Adapter Error "
			       "code = 0x%x\n", aha_name(aha),
			       aha_inb(aha, DATAIN_REG));
		return (ENXIO);
	}

	/* If we've allocated mailboxes, initialize them */
	if (aha->init_level > 4)
		ahainitmboxes(aha);

	/* If we've attached to the XPT, tell it about the event */
	if (aha->path != NULL)
		xpt_async(AC_BUS_RESET, aha->path, NULL);

	/*
	 * Perform completion processing for all outstanding CCBs.
	 */
	while ((ccb_h = LIST_FIRST(&aha->pending_ccbs)) != NULL) {
		struct aha_ccb *pending_bccb;

		pending_bccb = (struct aha_ccb *)ccb_h->ccb_bccb_ptr;
		pending_bccb->hccb.ahastat = AHASTAT_HA_SCSI_BUS_RESET;
		ahadone(aha, pending_bccb, BMBI_ERROR);
	}

	return (0);
}

/*
 * Send a command to the adapter.
 */
int
aha_cmd(struct aha_softc *aha, aha_op_t opcode, u_int8_t *params, 
      u_int param_len, u_int8_t *reply_data, u_int reply_len, 
      u_int cmd_timeout)
{
	u_int	timeout;
	u_int	status;
	u_int	intstat;
	u_int	reply_buf_size;
	int	s;

	/* No data returned to start */
	reply_buf_size = reply_len;
	reply_len = 0;
	intstat = 0;

	aha->command_cmp = 0;
	/*
	 * Wait up to 1 sec. for the adapter to become
	 * ready to accept commands.
	 */
	timeout = 10000;
	while (--timeout) {

		status = aha_inb(aha, STATUS_REG);
		if ((status & HA_READY) != 0
		 && (status & CMD_REG_BUSY) == 0)
			break;
		DELAY(100);
	}
	if (timeout == 0) {
		printf("%s: aha_cmd: Timeout waiting for adapter ready, "
		       "status = 0x%x\n", aha_name(aha), status);
		return (ETIMEDOUT);
	}

	/*
	 * Send the opcode followed by any necessary parameter bytes.
	 */
	aha_outb(aha, COMMAND_REG, opcode);

	/*
	 * Wait for up to 1sec to get the parameter list sent
	 */
	timeout = 10000;
	while (param_len && --timeout) {
		DELAY(100);
		status = aha_inb(aha, STATUS_REG);
		intstat = aha_inb(aha, INTSTAT_REG);
		if ((intstat & (INTR_PENDING|CMD_COMPLETE))
		 == (INTR_PENDING|CMD_COMPLETE))
			break;
		if (aha->command_cmp != 0) {
			status = aha->latched_status;
			break;
		}
		if ((status & DATAIN_REG_READY) != 0)
			break;
		if ((status & CMD_REG_BUSY) == 0) {
			aha_outb(aha, COMMAND_REG, *params++);
			param_len--;
		}
	}
	if (timeout == 0) {
		printf("%s: aha_cmd: Timeout sending parameters, "
		       "status = 0x%x\n", aha_name(aha), status);
		return (ETIMEDOUT);
	}

	/*
	 * For all other commands, we wait for any output data
	 * and the final comand completion interrupt.
	 */
	while (--cmd_timeout) {

		status = aha_inb(aha, STATUS_REG);
		intstat = aha_inb(aha, INTSTAT_REG);
		if ((intstat & (INTR_PENDING|CMD_COMPLETE))
		 == (INTR_PENDING|CMD_COMPLETE))
			break;

		if (aha->command_cmp != 0) {
			status = aha->latched_status;
			break;
		}

		if ((status & DATAIN_REG_READY) != 0) {
			u_int8_t data;

			data = aha_inb(aha, DATAIN_REG);
			if (reply_len < reply_buf_size) {
				*reply_data++ = data;
			} else {
				printf("%s: aha_cmd - Discarded reply data byte "
				       "for opcode 0x%x\n", aha_name(aha),
				       opcode);
			}
			reply_len++;
		}

		DELAY(100);
	}
	if (timeout == 0) {
		printf("%s: aha_cmd: Timeout waiting for reply data and "
		       "command complete.\n%s: status = 0x%x, intstat = 0x%x, "
		       "reply_len = %d\n", aha_name(aha), aha_name(aha), status,
		       intstat, reply_len);
		return (ETIMEDOUT);
	}

	/*
	 * Clear any pending interrupts.  Block interrupts so our
	 * interrupt handler is not re-entered.
	 */
	s = splcam();
	aha_intr(aha);
	splx(s);
	
	/*
	 * If the command was rejected by the controller, tell the caller.
	 */
	if ((status & CMD_INVALID) != 0) {
		if (bootverbose)
			printf("%s: Invalid Command 0x%x\n", aha_name(aha), 
				opcode);
		/*
		 * Some early adapters may not recover properly from
		 * an invalid command.  If it appears that the controller
		 * has wedged (i.e. status was not cleared by our interrupt
		 * reset above), perform a soft reset.
      		 */
		DELAY(1000);
		status = aha_inb(aha, STATUS_REG);
		if ((status & (CMD_INVALID|STATUS_REG_RSVD|DATAIN_REG_READY|
			      CMD_REG_BUSY|DIAG_FAIL|DIAG_ACTIVE)) != 0
		 || (status & (HA_READY|INIT_REQUIRED))
		  != (HA_READY|INIT_REQUIRED)) {
			ahareset(aha, /*hard_reset*/FALSE);
		}
		return (EINVAL);
	}

	
	if (param_len > 0) {
		/* The controller did not accept the full argument list */
	 	return (E2BIG);
	}

	if (reply_len != reply_buf_size) {
		/* Too much or too little data received */
		return (EMSGSIZE);
	}

	/* We were successful */
	return (0);
}

static int
ahainitmboxes(struct aha_softc *aha) 
{
	int error;
	init_24b_mbox_params_t init_mbox;

	bzero(aha->in_boxes, sizeof(aha_mbox_in_t) * aha->num_boxes);
	bzero(aha->out_boxes, sizeof(aha_mbox_out_t) * aha->num_boxes);
	aha->cur_inbox = aha->in_boxes;
	aha->last_inbox = aha->in_boxes + aha->num_boxes - 1;
	aha->cur_outbox = aha->out_boxes;
	aha->last_outbox = aha->out_boxes + aha->num_boxes - 1;

	/* Tell the adapter about them */
	init_mbox.num_mboxes = aha->num_boxes;
	ahautoa24(aha->mailbox_physbase, init_mbox.base_addr);
	error = aha_cmd(aha, BOP_INITIALIZE_MBOX, (u_int8_t *)&init_mbox,
		       /*parmlen*/sizeof(init_mbox), /*reply_buf*/NULL,
		       /*reply_len*/0, DEFAULT_CMD_TIMEOUT);

	if (error != 0)
		printf("ahainitmboxes: Initialization command failed\n");
	return (error);
}

/*
 * Update the XPT's idea of the negotiated transfer
 * parameters for a particular target.
 */
static void
ahafetchtransinfo(struct aha_softc *aha, struct ccb_trans_settings* cts)
{
	setup_data_t	setup_info;
	u_int		target;
	u_int		targ_offset;
	u_int		sync_period;
	int		error;
	u_int8_t	param;
	targ_syncinfo_t	sync_info;

	target = cts->ccb_h.target_id;
	targ_offset = (target & 0x7);

	/*
	 * Inquire Setup Information.  This command retreives the
	 * Wide negotiation status for recent adapters as well as
	 * the sync info for older models.
	 */
	param = sizeof(setup_info);
	error = aha_cmd(aha, BOP_INQUIRE_SETUP_INFO, &param, /*paramlen*/1,
		       (u_int8_t*)&setup_info, sizeof(setup_info),
		       DEFAULT_CMD_TIMEOUT);

	if (error != 0) {
		printf("%s: ahafetchtransinfo - Inquire Setup Info Failed\n",
		       aha_name(aha));
		return;
	}

	sync_info = setup_info.syncinfo[targ_offset];

	if (sync_info.sync == 0)
		cts->sync_offset = 0;
	else
		cts->sync_offset = sync_info.offset;

	cts->bus_width = MSG_EXT_WDTR_BUS_8_BIT;

	sync_period = 2000 + (500 * sync_info.period);

	/* Convert ns value to standard SCSI sync rate */
	if (cts->sync_offset != 0)
		cts->sync_period = scsi_calc_syncparam(sync_period);
	else
		cts->sync_period = 0;
	
	cts->valid = CCB_TRANS_SYNC_RATE_VALID
		   | CCB_TRANS_SYNC_OFFSET_VALID
		   | CCB_TRANS_BUS_WIDTH_VALID;
        xpt_async(AC_TRANSFER_NEG, cts->ccb_h.path, cts);
}

static void
ahamapmboxes(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	struct aha_softc* aha;

	aha = (struct aha_softc*)arg;
	aha->mailbox_physbase = segs->ds_addr;
}

static void
ahamapccbs(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{
	struct aha_softc* aha;

	aha = (struct aha_softc*)arg;
	aha->aha_ccb_physbase = segs->ds_addr;
}

static void
ahamapsgs(void *arg, bus_dma_segment_t *segs, int nseg, int error)
{

	struct aha_softc* aha;

	aha = (struct aha_softc*)arg;
	SLIST_FIRST(&aha->sg_maps)->sg_physaddr = segs->ds_addr;
}

static void
ahapoll(struct cam_sim *sim)
{
}

void
ahatimeout(void *arg)
{
	struct aha_ccb	*bccb;
	union  ccb	*ccb;
	struct aha_softc *aha;
	int		 s;
	u_int32_t	paddr;

	bccb = (struct aha_ccb *)arg;
	ccb = bccb->ccb;
	aha = (struct aha_softc *)ccb->ccb_h.ccb_aha_ptr;
	xpt_print_path(ccb->ccb_h.path);
	printf("CCB %p - timed out\n", (void *)bccb);

	s = splcam();

	if ((bccb->flags & BCCB_ACTIVE) == 0) {
		xpt_print_path(ccb->ccb_h.path);
		printf("CCB %p - timed out CCB already completed\n",
		       (void *)bccb);
		splx(s);
		return;
	}

	/*
	 * In order to simplify the recovery process, we ask the XPT
	 * layer to halt the queue of new transactions and we traverse
	 * the list of pending CCBs and remove their timeouts. This
	 * means that the driver attempts to clear only one error
	 * condition at a time.  In general, timeouts that occur
	 * close together are related anyway, so there is no benefit
	 * in attempting to handle errors in parrallel.  Timeouts will
	 * be reinstated when the recovery process ends.
	 */
	if ((bccb->flags & BCCB_DEVICE_RESET) == 0) {
		struct ccb_hdr *ccb_h;

		if ((bccb->flags & BCCB_RELEASE_SIMQ) == 0) {
			xpt_freeze_simq(aha->sim, /*count*/1);
			bccb->flags |= BCCB_RELEASE_SIMQ;
		}

		ccb_h = LIST_FIRST(&aha->pending_ccbs);
		while (ccb_h != NULL) {
			struct aha_ccb *pending_bccb;

			pending_bccb = (struct aha_ccb *)ccb_h->ccb_bccb_ptr;
			untimeout(ahatimeout, pending_bccb, ccb_h->timeout_ch);
			ccb_h = LIST_NEXT(ccb_h, sim_links.le);
		}
	}

	if ((bccb->flags & BCCB_DEVICE_RESET) != 0
	 || aha->cur_outbox->action_code != BMBO_FREE) {
		/*
		 * Try a full host adapter/SCSI bus reset.
		 * We do this only if we have already attempted
		 * to clear the condition with a BDR, or we cannot
		 * attempt a BDR for lack of mailbox resources.
		 */
		ccb->ccb_h.status = CAM_CMD_TIMEOUT;
		ahareset(aha, /*hardreset*/TRUE);
		printf("%s: No longer in timeout\n", aha_name(aha));
	} else {
		/*    
		 * Send a Bus Device Reset message:
		 * The target that is holding up the bus may not
		 * be the same as the one that triggered this timeout
		 * (different commands have different timeout lengths),
		 * but we have no way of determining this from our
		 * timeout handler.  Our strategy here is to queue a
		 * BDR message to the target of the timed out command.
		 * If this fails, we'll get another timeout 2 seconds
		 * later which will attempt a bus reset.
		 */
		bccb->flags |= BCCB_DEVICE_RESET;
		ccb->ccb_h.timeout_ch = timeout(ahatimeout, (caddr_t)bccb, 2 * hz);
		aha->recovery_bccb->hccb.opcode = INITIATOR_BUS_DEV_RESET;

		/* No Data Transfer */
		aha->recovery_bccb->hccb.datain = TRUE;
		aha->recovery_bccb->hccb.dataout = TRUE;
		aha->recovery_bccb->hccb.ahastat = 0;
		aha->recovery_bccb->hccb.sdstat = 0;
		aha->recovery_bccb->hccb.target = ccb->ccb_h.target_id;

		/* Tell the adapter about this command */
		paddr = ahaccbvtop(aha, aha->recovery_bccb);
		ahautoa24(paddr, aha->cur_outbox->ccb_addr);
		aha->cur_outbox->action_code = BMBO_START;
		aha_outb(aha, COMMAND_REG, BOP_START_MBOX);
		ahanextoutbox(aha);
	}

	splx(s);
}

/*
 * Copyright (c) 2001 by David Brownell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* this file is part of ehci-hcd.c */

/*-------------------------------------------------------------------------*/

/*
 * There's basically three types of memory:
 *	- data used only by the HCD ... kmalloc is fine
 *	- async and periodic schedules, shared by HC and HCD ... these
 *	  need to use dma_pool or hal_malloc_coherent
 *	- driver buffers, read/written by HC ... single shot DMA mapped
 *
 * There's also "register" data (e.g. PCI or SOC), which is memory mapped.
 * No memory seen by this driver is pageable.
 */

/*-------------------------------------------------------------------------*/

/* Allocate the key transfer structures from the previously allocated pool */

static inline void ehci_qtd_init(struct ehci_hcd *ehci, struct ehci_qtd *qtd, dma_addr_t dma)
{
	memset(qtd, 0, sizeof *qtd);
	qtd->qtd_dma = dma;
	qtd->hw_token = cpu_to_hc32(ehci, QTD_STS_HALT);
	qtd->hw_next = EHCI_LIST_END(ehci);
	qtd->hw_alt_next = EHCI_LIST_END(ehci);
	INIT_LIST_HEAD(&qtd->qtd_list);
	hal_dcache_clean_invalidate((unsigned long)qtd->qtd_dma, sizeof(struct ehci_qtd));
}

static struct ehci_qtd *ehci_qtd_alloc(struct ehci_hcd *ehci, gfp_t flags)
{
	struct ehci_qtd *qtd;
	dma_addr_t dma;

	// qtd = dma_pool_alloc (ehci->qtd_pool, flags, &dma);
	qtd = usb_dma_malloc(sizeof(struct ehci_qtd), &dma, USB_DESC_MALLOC_ALIGN_SIZE);
	if (qtd != NULL)
		ehci_qtd_init(ehci, qtd, dma);

	return qtd;
}

static inline void ehci_qtd_free(struct ehci_hcd *ehci, struct ehci_qtd *qtd)
{
	// dma_pool_free (ehci->qtd_pool, qtd, qtd->qtd_dma);
	usb_dma_free(qtd, qtd->qtd_dma);
}

static void qh_destroy(struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	/* clean qtds first, and know this is not linked */
	if (!list_empty(&qh->qtd_list) || qh->qh_next.ptr) {
		ehci_dbg("unused qh not empty!\n");
	}
	if (qh->dummy)
		ehci_qtd_free(ehci, qh->dummy);
	// dma_pool_free(ehci->qh_pool, qh->hw, qh->qh_dma);
	usb_dma_free((void *)qh->hw, qh->qh_dma);
	hal_free(qh);
}

static struct ehci_qh *ehci_qh_alloc(struct ehci_hcd *ehci)
{
	struct ehci_qh *qh;
	dma_addr_t dma;

	qh = hal_malloc(sizeof *qh);
	if (!qh)
		goto done;
	memset(qh, 0, sizeof(struct ehci_qh));
	// qh->hw = (struct ehci_qh_hw *)
	//	dma_pool_alloc(ehci->qh_pool, flags, &dma);
	qh->hw = (struct ehci_qh_hw *)usb_dma_malloc(sizeof(struct ehci_qh_hw), &dma,
						     USB_DESC_MALLOC_ALIGN_SIZE);
	if (!qh->hw)
		goto fail;
	memset(qh->hw, 0, sizeof *qh->hw);
	qh->qh_dma = dma;
	INIT_LIST_HEAD(&qh->qtd_list);
	INIT_LIST_HEAD(&qh->unlink_node);

	/* dummy td enables safe urb queuing */
	qh->dummy = ehci_qtd_alloc(ehci, 0);
	if (qh->dummy == NULL) {
		ehci_dbg("no dummy td\n");
		goto fail1;
	}
done:
	return qh;
fail1:
	// dma_pool_free(ehci->qh_pool, qh->hw, qh->qh_dma);
	usb_dma_free(qh->hw, qh->qh_dma);
fail:
	// kfree(qh);
	hal_free(qh);
	return NULL;
}

/*-------------------------------------------------------------------------*/

/* The queue heads and transfer descriptors are managed from pools tied
 * to each of the "per device" structures.
 * This is the initialisation and cleanup code.
 */

static void ehci_mem_cleanup(struct ehci_hcd *ehci)
{
	if (ehci->async)
		qh_destroy(ehci, ehci->async);
	ehci->async = NULL;

	// if (ehci->dummy)
	//	qh_destroy(ehci, ehci->dummy);
	// ehci->dummy = NULL;

	/* DMA consistent memory and pools */
	// dma_pool_destroy(ehci->qtd_pool);
	// ehci->qtd_pool = NULL;
	// dma_pool_destroy(ehci->qh_pool);
	// ehci->qh_pool = NULL;
	// dma_pool_destroy(ehci->itd_pool);
	// ehci->itd_pool = NULL;
	// dma_pool_destroy(ehci->sitd_pool);
	// ehci->sitd_pool = NULL;

	if (ehci->periodic)
		usb_dma_free(ehci->periodic, ehci->periodic_dma);
	// hal_free_coherent (ehci_to_hcd(ehci)->self.controller,
	//	ehci->periodic_size * sizeof (u32),
	//	ehci->periodic, ehci->periodic_dma);
	ehci->periodic = NULL;

	/* shadow periodic table */
	hal_free(ehci->pshadow);
	ehci->pshadow = NULL;
}

/* remember to add cleanup code (above) if you add anything here */
static int ehci_mem_init(struct ehci_hcd *ehci)
{
	int i;
	int pshadow_size = (ehci->periodic_size) * (sizeof(void *));

	/* QTDs for control/bulk/intr transfers */
	// ehci->qtd_pool = dma_pool_create ("ehci_qtd",
	//		ehci_to_hcd(ehci)->self.controller,
	//		sizeof (struct ehci_qtd),
	//		32 /* byte alignment (for hw parts) */,
	//		4096 /* can't cross 4K */);
	// if (!ehci->qtd_pool) {
	//	goto fail;
	//}

	///* QHs for control/bulk/intr transfers */
	// ehci->qh_pool = dma_pool_create ("ehci_qh",
	//		ehci_to_hcd(ehci)->self.controller,
	//		sizeof(struct ehci_qh_hw),
	//		32 /* byte alignment (for hw parts) */,
	//		4096 /* can't cross 4K */);
	// if (!ehci->qh_pool) {
	//	goto fail;
	//}
	ehci->async = ehci_qh_alloc(ehci);
	if (!ehci->async) {
		goto fail;
	}

	/* ITD for high speed ISO transfers */
	// ehci->itd_pool = dma_pool_create ("ehci_itd",
	//		ehci_to_hcd(ehci)->self.controller,
	//		sizeof (struct ehci_itd),
	//		32 /* byte alignment (for hw parts) */,
	//		4096 /* can't cross 4K */);
	// if (!ehci->itd_pool) {
	//	goto fail;
	//}

	///* SITD for full/low speed split ISO transfers */
	// ehci->sitd_pool = dma_pool_create ("ehci_sitd",
	//		ehci_to_hcd(ehci)->self.controller,
	//		sizeof (struct ehci_sitd),
	//		32 /* byte alignment (for hw parts) */,
	//		4096 /* can't cross 4K */);
	// if (!ehci->sitd_pool) {
	//	goto fail;
	//}

	/* Hardware periodic table */
	// ehci->periodic = (__le32 *)
	//	hal_malloc_coherent (ehci_to_hcd(ehci)->self.controller,
	//		ehci->periodic_size * sizeof(__le32),
	//		&ehci->periodic_dma, flags);
	ehci->periodic = usb_dma_malloc(ehci->periodic_size * sizeof(uint32_t),
						   (dma_addr_t *)&ehci->periodic_dma, USB_EHCI_PERIODIC_MALLOC_ALIGN_SIZE);
	if (ehci->periodic == NULL) {
		goto fail;
	}

	// if (ehci->use_dummy_qh) {
	//	struct ehci_qh_hw	*hw;
	//	ehci->dummy = ehci_qh_alloc(ehci, flags);
	//	if (!ehci->dummy)
	//		goto fail;

	//	hw = ehci->dummy->hw;
	//	hw->hw_next = EHCI_LIST_END(ehci);
	//	hw->hw_qtd_next = EHCI_LIST_END(ehci);
	//	hw->hw_alt_next = EHCI_LIST_END(ehci);
	//	ehci->dummy->hw = hw;

	//	for (i = 0; i < ehci->periodic_size; i++)
	//		ehci->periodic[i] = cpu_to_hc32(ehci,
	//				ehci->dummy->qh_dma);
	//} else {
	for (i = 0; i < ehci->periodic_size; i++)
		ehci->periodic[i] = EHCI_LIST_END(ehci);
	//}

	/* software shadow of hardware table */
	// ehci->pshadow = kcalloc(ehci->periodic_size, sizeof(void *), flags);
	ehci->pshadow = hal_malloc(pshadow_size);
	memset(ehci->pshadow, 0, pshadow_size);
	if (ehci->pshadow != NULL)
		return 0;

fail:
	ehci_dbg("couldn't init memory\n");
	ehci_mem_cleanup(ehci);
	return -ENOMEM;
}

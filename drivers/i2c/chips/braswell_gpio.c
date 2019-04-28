/*
 *  Copyright (C) 2009 Thecus Technology Corp.
 *
 *      Maintainer: citizen <joey_wang@thecus.com>
 *
 *      Driver for braswell GPIO on Thecus N2810 Board
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/delay.h>

#define NAME	"braswell_gpio"

MODULE_AUTHOR("Joey Wang");
MODULE_DESCRIPTION("Intel braswell GPIO Driver for Thecus N5810 Board");
MODULE_LICENSE("GPL");

/* Module and version information */
#define GPIO_VERSION "20151116"
#define GPIO_MODULE_NAME "Intel braswell GPIO driver"
#define GPIO_DRIVER_NAME   GPIO_MODULE_NAME ", v" GPIO_VERSION

//#define DEBUG 1

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (DEBUG>=x) printk(NAME ": %s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

static struct pci_dev *braswell_gpio_pci = NULL;

/*
 * Data for PCI driver interface
 *
 * This data only exists for exporting the supported
 * PCI ids via MODULE_DEVICE_TABLE.  We do not actually
 * register a pci_driver, because someone else might one day
 * want to register another driver on the same PCI id.
 */
static struct pci_device_id braswell_gpio_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x229c)}, /* braswell */
    {0,},			/* End of list */
};

MODULE_DEVICE_TABLE(pci, braswell_gpio_pci_tbl);

static int braswell_gpio_probe(struct pci_dev *, const struct pci_device_id *);

static struct pci_driver braswell_gpio_pci_driver = {
        .name = "braswell_gpio",
        .id_table = braswell_gpio_pci_tbl,
        .probe = braswell_gpio_probe,
};

/*
 *	Init & exit routines
 */
static unsigned char __init braswell_gpio_getdevice(struct pci_dev *dev)
{
    u8 val1, val2;
    u32 badr;
    u32 gval, sval;


    printk(KERN_INFO NAME ": Braswell PCI Vendor [%X] DEVICE [%X]\n", dev->subsystem_vendor,
	   dev->subsystem_device);

    if (braswell_gpio_pci) {
	printk(KERN_INFO NAME ": Braswell GPIO has already configured\n");
        return 0;
    }else{
	braswell_gpio_pci=1;
    }
    
}

static int braswell_gpio_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    /* Check whether or not the braswell LPC is there */
    if (!braswell_gpio_getdevice(pdev) || braswell_gpio_pci == NULL)
	return -ENODEV;

    return 0;
}

int braswell_GPIO_init(void)
{
    struct pci_dev *pdev = NULL;
    const struct pci_device_id *ent = NULL;

    for_each_pci_dev(pdev) {
      ent = pci_match_id(braswell_gpio_pci_tbl, pdev);

      if (ent) break;
    }

    if(ent == NULL) return -1;

    printk(KERN_INFO NAME ": %s\n", GPIO_DRIVER_NAME);

    return pci_register_driver(&braswell_gpio_pci_driver);
}

void braswell_GPIO_exit(void)
{
    // Deregister
    pci_unregister_driver(&braswell_gpio_pci_driver);
}


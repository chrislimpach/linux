/*
 *  Copyright (C) 2009 Thecus Technology Corp.
 *
 *      Maintainer: citizen <joey_wang@thecus.com>
 *
 *      Driver for Baytrail SOC GPIO on Thecus N2800 Board
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

#include "soc_gpio.h"

#define NAME	"soc_gpio"

MODULE_AUTHOR("Joey Wang");
MODULE_DESCRIPTION("Intel Baytrail SOC GPIO Driver for Thecus N5810 Board");
MODULE_LICENSE("GPL");

/* Module and version information */
#define GPIO_VERSION "20140825"
#define GPIO_MODULE_NAME "Intel Baytrail SOC GPIO driver"
#define GPIO_DRIVER_NAME   GPIO_MODULE_NAME ", v" GPIO_VERSION

//#define DEBUG 1

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (DEBUG>=x) printk(NAME ": %s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

/* internal variables */
static u32 GPIO_ADDR = 0, PM_ADDR = 0;//, LPC_ADDR = 0;

static struct pci_dev *soc_gpio_pci = NULL;

extern int check_bit(u32 val, int bn);
extern void print_gpio(u32 gpio_val, char *zero_str, char *one_str, u32 offset);
/*
Parameters:
bit_n: bit# to read 
Return value:
1: High
0: Low
-1: Error
*/
int SOC_gpio_read_bit(u32 bit_n)
{
    int ret = 0;
    u32 gval = 0;

    if (GPIO_ADDR == 0) {
	//printk(KERN_ERR NAME ": GPIO_ADDR is NULL\n");
	return -1;
    }
/*
    if (bit_n < 32) {		//check GP_LVL
	gval = inl(GPIO_ADDR + GP_LVL);
	ret = check_bit(gval, bit_n);
    } else if ((bit_n >= 32) && (bit_n <= 63)) {	//check GP_LVL2
	gval = inl(GPIO_ADDR + GP_LVL2);
	ret = check_bit(gval, bit_n - 32);
    } else if ((bit_n >= 64) && (bit_n <= 95)) {	//check GP_LVL3
	gval = inl(GPIO_ADDR + GP_LVL3);
	ret = check_bit(gval, bit_n - 64);
    } else
	ret = -1;
*/
    //if(ret>=0)
    //printk("Read=0x%08X, bit[%d]=%d\n",gval,bit_n,ret);

    return ret;
}

/*
Parameters:
bit_n: bit# to update
val: [0/1] values to update 
Return value:(after set)
1: High 
0: Low
-1: Error
*/
int SOC_gpio_write_bit(u32 bit_n, int val)
{
    int ret = 0;
    u32 gval, sval, mask_val;
    mask_val = 1;
    sval = val;

    if (GPIO_ADDR == 0) {
	//printk(KERN_ERR NAME ": GPIO_ADDR is NULL\n");
	return -1;
    }
/*
    if (bit_n < 32) {		//check GP_LVL
	gval = inl(GPIO_ADDR + GP_LVL);
	mask_val = mask_val << bit_n;
	sval = sval << bit_n;
	gval = (gval & ~mask_val) | sval;
	outl(gval, GPIO_ADDR + GP_LVL);
    } else if ((bit_n >= 32) && (bit_n <= 63)) {	//check GP_LVL2
	gval = inl(GPIO_ADDR + GP_LVL2);
	mask_val = mask_val << (bit_n - 32);
	sval = sval << (bit_n - 32);
	gval = (gval & ~mask_val) | sval;
	outl(gval, GPIO_ADDR + GP_LVL2);
    } else if ((bit_n >= 64) && (bit_n <= 95)) {	//check GP_LVL3
	gval = inl(GPIO_ADDR + GP_LVL3);
	mask_val = mask_val << (bit_n - 64);
	sval = sval << (bit_n - 64);
	gval = (gval & ~mask_val) | sval;
	outl(gval, GPIO_ADDR + GP_LVL3);
    } else
	ret = -1;
*/
    return ret;
}

/*
 * Data for PCI driver interface
 *
 * This data only exists for exporting the supported
 * PCI ids via MODULE_DEVICE_TABLE.  We do not actually
 * register a pci_driver, because someone else might one day
 * want to register another driver on the same PCI id.
 */
static struct pci_device_id soc_gpio_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x0f1c)}, /* baytrail SOC*/
    {0,},			/* End of list */
};

MODULE_DEVICE_TABLE(pci, soc_gpio_pci_tbl);

static int soc_gpio_probe(struct pci_dev *, const struct pci_device_id *);

static struct pci_driver soc_gpio_pci_driver = {
        .name = "soc_gpio",
        .id_table = soc_gpio_pci_tbl,
        .probe = soc_gpio_probe,
};

/*
 *	Init & exit routines
 */
static unsigned char __init soc_gpio_getdevice(struct pci_dev *dev)
{
    u8 val1, val2;
    u32 badr;
    u32 gval, sval;


    printk(KERN_INFO NAME ": SOC PCI Vendor [%X] DEVICE [%X]\n", dev->subsystem_vendor,
	   dev->subsystem_device);

    if (soc_gpio_pci) {
	printk(KERN_INFO NAME ": SOC GPIO has already configured\n");
        return 0;
    }else{
	soc_gpio_pci=1;
    }

}

static int soc_gpio_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    int ret;

    /* Check whether or not the SOC LPC is there */
    if (!soc_gpio_getdevice(pdev) || soc_gpio_pci == NULL)
	return -ENODEV;
/*
    if (!request_region(GPIO_ADDR, GPIO_IO_PORTS, "SOC GPIO")) {
	printk(KERN_ERR NAME ": I/O address 0x%04x already in use\n", GPIO_ADDR);
	ret = -EIO;
	goto out;
    }
*/
    return 0;

  out:
    return ret;
}

int SOC_GPIO_init(void)
{
    struct pci_dev *pdev = NULL;
    const struct pci_device_id *ent = NULL;

    for_each_pci_dev(pdev) {
      ent = pci_match_id(soc_gpio_pci_tbl, pdev);

      if (ent) break;
    }

    if(ent == NULL) return -1;

    printk(KERN_INFO NAME ": %s\n", GPIO_DRIVER_NAME);

    return pci_register_driver(&soc_gpio_pci_driver);
}

void SOC_GPIO_exit(void)
{
    // Deregister
    pci_unregister_driver(&soc_gpio_pci_driver);
/*
    if (GPIO_ADDR != 0) release_region(GPIO_ADDR, GPIO_IO_PORTS);
*/
}


#include <linux/module.h>
#include <linux/pci.h>

#include "pch_gpio.h"

#define NAME "pch_8_gpio"
/* Module and version information*/
#define GPIO_VERSION "20150527"
#define GPIO_MODULE_NAME "Intel C220 / C230 GPIO driver"
#define GPIO_DRIVER_NAME   GPIO_MODULE_NAME ", v" GPIO_VERSION

static struct pci_dev *pch_8_gpio_pci = NULL;

static struct pci_device_id pch_8_gpio_pci_tbl[] = {
    {PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x8c54)}, /* C22424 */
    {PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0xa149)}, /* C230 Sunrise Point-H LPC Controller */
    {0,},
};

MODULE_DEVICE_TABLE(pci, pch_8_gpio_pci_tbl);

static int pch_8_gpio_probe(struct pci_dev *, const struct pci_device_id *);

static struct pci_driver pch_8_gpio_pci_driver = {
	.name = "pch_8_gpio",
	.id_table = pch_8_gpio_pci_tbl,
	.probe = pch_8_gpio_probe,
};

/*
 *	Init & exit routines
 */
static unsigned int __init pch_8_gpio_getdevice(struct pci_dev *pdev)
{
	printk(KERN_INFO NAME ": SOC PCI Vendor [%X] DEVICE [%X]\n", pdev->subsystem_vendor,
		pdev->subsystem_device);

	if(pch_8_gpio_pci){
		printk(KERN_INFO NAME ": PCH GPIO has already configured\n");
		return 0;
	}else{
		return 1;
	}
}

static int pch_8_gpio_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	/* Check whether the PCH LPC exists or not*/
	if(!pch_8_gpio_getdevice(pdev) || pch_8_gpio_pci == NULL)
		return -ENODEV;

	return 0;
}

int pch_8_gpio_init(void)
{
	struct pci_dev *pdev = NULL;
	const struct pci_device_id *ent = NULL;

	for_each_pci_dev(pdev){
		ent = pci_match_id(pch_8_gpio_pci_tbl ,pdev);
		if (ent) break;
	}

	if (ent ==  NULL) return -1;
	printk(KERN_INFO NAME": %s\n", GPIO_DRIVER_NAME);

	return pci_register_driver(&pch_8_gpio_pci_driver);
}

void pch_8_gpio_exit(void)
{
	pci_unregister_driver(&pch_8_gpio_pci_driver);
}

#ifndef _TN4022_DEV_H
#define _TN4022_DEV_H

#define BDX_DRV_VERSION   	"1.1.6.8_D"
#define DRIVER_AUTHOR     	"Tehuti networks"
#define BDX_DRV_DESC      	"Tehuti Network Driver"
#define BDX_DRV_NAME      	"tn4022"
#define BDX_NIC_NAME      	"tn4022"


// Supported PHYs:

#define PHY_QT2025

#include "tn40.h"

unsigned short QT2025_register(struct bdx_priv *priv);
unsigned short CX4_register(struct bdx_priv *priv);

#define LDEV(_vid,_pid,_subdev,_msi,_ports,_phya,_phyb,_name)  \
    {_vid,_pid,_subdev,_msi,_ports,PHY_TYPE_##_phya,PHY_TYPE_##_phyb}

static struct bdx_device_descr  bdx_dev_tbl[] =
{
    LDEV(TEHUTI_VID,0x4010,0x4010,1,1,CX4,NA,"GIZA Clean srom"),
    LDEV(TEHUTI_VID,0x4022,0x4D00,1,1,QT2025,NA,"GIZA SFP+"),
    {0}
};
#define PCI_REC(_vid,_pid) {_vid,_pid, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0}

static struct pci_device_id bdx_pci_tbl[] =
{
	{TEHUTI_VID, 0x4010, TEHUTI_VID, 0x4010, 0, 0, 0},
	{TEHUTI_VID, 0x4022, 0x1186,     0x4d00, 0, 0, 0},
	{0}
};

#endif // _TN4022_DEV_H

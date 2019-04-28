#ifndef _TN40XX_DEV_H
#define _TN40XX_DEV_H

#define BDX_DRV_VERSION   	"0.3.6.11"
#define DRIVER_AUTHOR     	"Tehuti networks"
#define BDX_DRV_DESC      	"Tehuti Network Driver"
#define BDX_DRV_NAME      	"tn40xx"
#define BDX_NIC_NAME      	"tn40xx"

// Supported PHYs:

#define PHY_MV88X3120
#define PHY_QT2025
#include "tn40.h"

unsigned short MV88X3120_register(struct bdx_priv *priv);
unsigned short TN8020_register(struct bdx_priv *priv);
unsigned short QT2025_register(struct bdx_priv *priv);
unsigned short CX4_register(struct bdx_priv *priv);

#define LDEV(_vid,_pid,_subdev,_msi,_ports,_phya,_phyb,_name)  \
    {_vid,_pid,_subdev,_msi,_ports,PHY_TYPE_##_phya,PHY_TYPE_##_phyb}

static struct bdx_device_descr  bdx_dev_tbl[] =
{
    LDEV(TEHUTI_VID,0x4010,0x4010,1,1,CX4,NA,"GIZA Clean srom"),
    LDEV(TEHUTI_VID,0x4020,0x3015,1,1,CX4,NA,"GIZA ES CX4"),
    LDEV(TEHUTI_VID,0x4022,0x3015,1,1,QT2025,NA,"GIZA SFP+"),
    LDEV(TEHUTI_VID,0x4024,0x3015,1,1,MV88X3120,NA,"GIZA 10GBASE-T Marvell"),
    {0}
};

static struct pci_device_id bdx_pci_tbl[] =
{
	{TEHUTI_VID, 0x4010, TEHUTI_VID, 0x4010, 0, 0, 0},
    {TEHUTI_VID, 0x4022, TEHUTI_VID, 0x3015, 0, 0, 0},
    {TEHUTI_VID, 0x4024, TEHUTI_VID, 0x3015, 0, 0, 0},
    {0}
};

#endif // _TN40XX_DEV_H

#include "tn40.h"

#define MV88X3310_EEE_10G			(0x0008)	//Support EEE for 10GBASE-T
#define MV88X3310_EEE_1G			(0x0004)	//Support EEE for 1GBASE-T
#define MV88X3310_EEE_100M			(0x0002)	//Support EEE for 100MBASE-T

//-------------------------------------------------------------------------------------------------

int MV88X3310_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   = (SUPPORTED_10000baseT_Full  | SUPPORTED_1000baseT_Full  | SUPPORTED_100baseT_Full  | SUPPORTED_Autoneg  | SUPPORTED_TP | SUPPORTED_Pause);
        if(!priv->advertising) {
		priv->advertising   = ecmd->supported;
		priv->autoneg     = AUTONEG_ENABLE;
	}
    ecmd->advertising   = priv->advertising;
	ecmd->speed       	=  priv->link_speed;
	ecmd->duplex      	= DUPLEX_FULL;
	ecmd->port        	= PORT_TP;
	ecmd->transceiver 	= XCVR_INTERNAL;
	ecmd->autoneg     	= priv->autoneg;
#if defined(ETH_TP_MDI_AUTO)
	ecmd->eth_tp_mdix 	= ETH_TP_MDI_AUTO;
#else
#if (!defined VM_KLNX)
	ecmd->eth_tp_mdix = ETH_TP_MDI | ETH_TP_MDI_X;
#endif
#endif
	return 0;

} // MV88X3310_get_settings()

#define	MV88X3310_10M_MASK		((1 << 5) | (1 << 6))	// reg7_0010
#define	MV88X3310_100M_MASK		(1 << 8)				// 		"
#define	MV88X3310_1G_MASK		(1 << 9)				// reg7_8000
#define	MV88X3310_2_5G_MASK		((1 << 5) | (1 << 7))	// reg7_0020
#define	MV88X3310_5G_MASK		((1 << 6) | (1 << 8))	// 		"
#define	MV88X3310_10G_MASK		(1 << 12)				// 		"
//-------------------------------------------------------------------------------------------------

int MV88X3310_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);
	u16 val, port=priv->phy_mdio_port;
	s32 speed=ethtool_cmd_speed(ecmd);
	u16 reg7_0010 = PHY_MDIO_READ(priv,7,0x0010);
	u16 reg7_0020 = PHY_MDIO_READ(priv,7,0x0020);
	u16 reg7_8000 = PHY_MDIO_READ(priv,7,0x8000);

	DBG("MV88X3310 ecmd->cmd=%x\n", ecmd->cmd);
	DBG("MV88X3310 speed=%u\n",speed);
	DBG("MV88X3310 ecmd->autoneg=%u\n",ecmd->autoneg);

	if(AUTONEG_ENABLE == ecmd->autoneg)
	{
		DBG("MV88X3310 speed 10G/1G/100m Autoneg\n");
        priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_Autoneg | ADVERTISED_Pause);
        priv->autoneg     = AUTONEG_ENABLE;
        reg7_0010 = (reg7_0010 & ~MV88X3310_10M_MASK) | MV88X3310_100M_MASK;
        reg7_0020 = reg7_0020 | MV88X3310_2_5G_MASK | MV88X3310_5G_MASK | MV88X3310_10G_MASK;
        reg7_8000 = reg7_8000 | MV88X3310_1G_MASK;

	}
	else
	{
		priv->autoneg     = AUTONEG_DISABLE;
		switch(speed)
		{
			case 10000: //10G
				DBG("MV88X3310 speed 10G\n");
				priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
		        reg7_0010 = reg7_0010 & ~MV88X3310_10M_MASK & ~MV88X3310_100M_MASK;
		        reg7_0020 = (reg7_0020 & ~MV88X3310_2_5G_MASK & ~MV88X3310_5G_MASK) | MV88X3310_10G_MASK;
		        reg7_8000 = reg7_8000 & ~MV88X3310_1G_MASK;
				break;

			case 5000: //5G
				DBG("MV88X3310 speed 5G\n");
				priv->advertising = (/* ADVERTISED_5000baseT_Full | */ ADVERTISED_Pause);
		        reg7_0010 = reg7_0010 & ~MV88X3310_10M_MASK & ~MV88X3310_100M_MASK;
		        reg7_0020 = (reg7_0020 & ~MV88X3310_2_5G_MASK & ~MV88X3310_10G_MASK) | MV88X3310_5G_MASK;
		        reg7_8000 = reg7_8000 & ~MV88X3310_1G_MASK;
				break;

			case 2500: //2.5G
				DBG("MV88X3310 speed 2.5G\n");
				priv->advertising = (/* ADVERTISED_10000baseT_Full | */ ADVERTISED_Pause);
		        reg7_0010 = reg7_0010 & ~MV88X3310_10M_MASK & ~MV88X3310_100M_MASK;
		        reg7_0020 = (reg7_0020 & ~MV88X3310_5G_MASK & ~MV88X3310_10G_MASK) | MV88X3310_2_5G_MASK;
		        reg7_8000 = reg7_0020 & ~MV88X3310_1G_MASK;
				break;

			case 1000:  //1G
				DBG("MV88X3310 speed 1G\n");
				priv->advertising = (ADVERTISED_1000baseT_Full | ADVERTISED_Pause);
		        reg7_0010 = reg7_0010 & ~MV88X3310_10M_MASK & ~MV88X3310_100M_MASK;
		        reg7_0020 = reg7_0020 & ~MV88X3310_2_5G_MASK & ~MV88X3310_10G_MASK & ~MV88X3310_5G_MASK;
		        reg7_8000 = reg7_8000 | MV88X3310_1G_MASK;
				break;

			case 100:   //100m
				DBG("MV88X3310 speed 100m\n");
				priv->advertising = (ADVERTISED_100baseT_Full | ADVERTISED_Pause);
		        reg7_0010 = (reg7_0010 & ~MV88X3310_10M_MASK) | MV88X3310_100M_MASK;
		        reg7_0020 = reg7_0020 & ~MV88X3310_2_5G_MASK & ~MV88X3310_10G_MASK & ~MV88X3310_5G_MASK;
		        reg7_8000 = reg7_8000 & ~MV88X3310_1G_MASK;
				break;

			default :
				ERR("does not support speed %u\n", speed);
				 return -EINVAL;
		}
	}
// set speed
	DBG("writing 7,0x0010 0x%04x 7,0x0020 0x%04x 7,0x8000 0x%04x\n", (u32)reg7_0010, (u32)reg7_0020, (u32)reg7_8000);
	BDX_MDIO_WRITE(priv, 7, 0x0010, reg7_0010);
	BDX_MDIO_WRITE(priv, 7, 0x0020, reg7_0020);
	BDX_MDIO_WRITE(priv, 7, 0x8000, reg7_8000);

// restart autoneg
	val = PHY_MDIO_READ(priv,7,0);
    val = val | 0x1200;
	BDX_MDIO_WRITE(priv, 0x07,0x0,val);

	return 0;

} // MV88X3310_set_settings()

#ifdef _EEE_
//----------------------------------------- EEE - IEEEaz ------------------------------------------
#ifdef ETHTOOL_GEEE
int MV88X3310_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
	ERR("EEE is not implemented yet\n");

	return -1;

} // MV88X3310_get_eee()
#endif
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_SEEE
int MV88X3310_set_eee(struct bdx_priv *priv)
{
	ERR("EEE is not implemented yet\n");

	return -1;

} // MV88X3310_set_eee()
#endif
//-------------------------------------------------------------------------------------------------

int MV88X3310_reset_eee(struct bdx_priv *priv)
{
	ERR("EEE is not implemented yet\n");

	return -1;

} // MV88X3310_reset_eee()

#endif
//-------------------------------------------------------------------------------------------------

__init void MV88X3310_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings = MV88X3310_get_settings;
    priv->phy_ops.set_settings = MV88X3310_set_settings;
#ifdef _EEE_
#ifdef ETHTOOL_GEEE
    priv->phy_ops.get_eee 	   = MV88X3310_get_eee;
#endif
#ifdef ETHTOOL_SEEE
    priv->phy_ops.set_eee 	   = MV88X3310_set_eee;
#endif
    priv->phy_ops.reset_eee    = MV88X3310_reset_eee;
#endif
} // MV88X3310_register_settings()

//-------------------------------------------------------------------------------------------------


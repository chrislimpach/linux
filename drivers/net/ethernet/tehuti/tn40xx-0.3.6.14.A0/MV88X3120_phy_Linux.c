#include "tn40.h"

#define MV88X3120_EEE_10G			(0x0008)	//Support EEE for 10GBASE-T
#define MV88X3120_EEE_1G			(0x0004)	//Support EEE for 1GBASE-T
#define MV88X3120_EEE_100M			(0x0002)	//Support EEE for 100MBASE-T

//-------------------------------------------------------------------------------------------------

int MV88X3120_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   = (SUPPORTED_10000baseT_Full  | SUPPORTED_1000baseT_Full  | SUPPORTED_100baseT_Full  | SUPPORTED_Autoneg  | SUPPORTED_TP | SUPPORTED_Pause);
        if(!priv->advertising) {
		priv->advertising   = ecmd->supported;
		priv->autoneg     = AUTONEG_ENABLE;
	}
    	ecmd->advertising   = priv->advertising;
	ecmd->speed       =  priv->link_speed;
	ecmd->duplex      = DUPLEX_FULL;
	ecmd->port        = PORT_TP;
	ecmd->transceiver = XCVR_INTERNAL;
	ecmd->autoneg     = priv->autoneg;
#if defined(ETH_TP_MDI_AUTO)
	ecmd->eth_tp_mdix = ETH_TP_MDI_AUTO;
#else
#if (!defined VM_KLNX)
	ecmd->eth_tp_mdix = ETH_TP_MDI | ETH_TP_MDI_X;
#endif
#endif
	return 0;

} // MV88X3120_get_settings()

//-------------------------------------------------------------------------------------------------

int MV88X3120_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);
	u16 val, port=priv->phy_mdio_port;
	s32 speed=ethtool_cmd_speed(ecmd);

#if 0 // debug
		ERR("MV88X3120 ecmd->cmd=%x\n", ecmd->cmd);
		ERR("MV88X3120 speed=%u\n",speed);
		ERR("MV88X3120 ecmd->autoneg=%u\n",ecmd->autoneg);
#endif

	if(AUTONEG_ENABLE == ecmd->autoneg)
	{
		DBG("MV88X3120 speed 10G/1G/100m Autoneg\n");
        val=(1<<3)|(1<<2)|(1<<1);
        priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_Autoneg | ADVERTISED_Pause);
        priv->autoneg     = AUTONEG_ENABLE;
	}
	else
	{
		priv->autoneg     = AUTONEG_DISABLE;
		switch(speed)
		{
			case 10000: //10G
				DBG("MV88X3120 speed 10G\n");
				val=(1<<3);
				priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
				break;

			case 1000:  //1G
				DBG("MV88X3120 speed 1G\n");
				val=(1<<2);
				priv->advertising = (ADVERTISED_1000baseT_Full | ADVERTISED_Pause);
				break;

			case 100:   //100m
				DBG("MV88X3120 speed 100m\n");
				val=(1<<1);
				priv->advertising = (ADVERTISED_100baseT_Full | ADVERTISED_Pause);
				break;

			default :
				ERR("does not support speed %u\n", speed);
				 return -EINVAL;
		}
	}
// set speed
	BDX_MDIO_WRITE(priv, 0x01,49192,val);

// restart autoneg
    val=(1<<12)|(1<<9)|(1<<13);
	BDX_MDIO_WRITE(priv, 0x07,0x0,val);

	return 0;

} // MV88X3120_set_settings()

//----------------------------------------- EEE - IEEEaz ------------------------------------------
#ifdef _EEE_

#ifdef ETHTOOL_GEEE
int MV88X3120_get_eee(struct net_device *netdev, struct ethtool_eee *edata)
{
    struct bdx_priv *priv = netdev_priv(netdev);
    u32	   val;

	/* EEE Capability */
	edata->supported = (SUPPORTED_10000baseT_Full) ;//  | SUPPORTED_1000baseT_Full  | SUPPORTED_100baseT_Full);
	/* EEE Advertised */
	val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 60);
	edata->advertised = 0;
	if(val & MV88X3120_EEE_10G) 	edata->advertised |= SUPPORTED_10000baseT_Full;
	/*if(val & MV88X3120_EEE_1G) 	edata->advertised |= SUPPORTED_1000baseT_Full;
	if(val & MV88X3120_EEE_100M) 	edata->advertised |= SUPPORTED_100baseT_Full;*/

	/* EEE Link Partner Advertised */
	val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 61);
	edata->lp_advertised = 0;
	if(val & MV88X3120_EEE_10G) 	edata->lp_advertised |= SUPPORTED_10000baseT_Full;
//	if(val & MV88X3120_EEE_1G) 		edata->lp_advertised |= SUPPORTED_1000baseT_Full;
//	if(val & MV88X3120_EEE_100M) 	edata->lp_advertised |= SUPPORTED_100baseT_Full;

	/* EEE Status */

	val=bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49192);
	if(val & 0x40) 
	edata->eee_active = true; //still need to check that link is up.

	edata->eee_enabled = true;
	//edata->tx_lpi_enabled = true;
//	edata->tx_lpi_timer = er32(LPIC) >> E1000_LPIC_LPIET_SHIFT;

return 0;

} // MV88X3120_get_eee()
#endif
//-------------------------------------------------------------------------------------------------
#ifdef ETHTOOL_SEEE
int MV88X3120_set_eee(struct bdx_priv *priv)
{
   // struct bdx_priv *priv = netdev_priv(netdev);
	u16 val, port=priv->phy_mdio_port;

	//edata->eee_active = true; //still need to check that link is up.
	//edata->eee_enabled = true;
	//EEE LPI Buffer Enable
    BDX_MDIO_WRITE(priv,1,49230, 0x0900);
    BDX_MDIO_WRITE(priv,1,49221, 0x1);
    BDX_MDIO_WRITE(priv,7,60, 0x8);
    /*val  = bdx_mdio_read(priv, 3, priv->phy_mdio_port, 20);
    val  = bdx_mdio_read(priv, 4, priv->phy_mdio_port, 20);
    val  = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 61);*/
	// restart autoneg
    val = (1<<12)|(1<<9)|(1<<13);
	BDX_MDIO_WRITE(priv,0x07,0x0,val);
    val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 1);
    val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 1);
    val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 60);
	ERR("DBG_EEE 7.60=0x%x \n", val);
    val = bdx_mdio_read(priv, 7, priv->phy_mdio_port, 61);
	ERR("DBG_EEE 7.61=0x%x \n", val);
    val = bdx_mdio_read(priv, 1, priv->phy_mdio_port, 49230);
	ERR("DBG_EEE 1.49230=0x%x\n", val);
	priv->eee_enabled=true;

	return 0;

} // MV88X3120_set_eee()

#endif

//-------------------------------------------------------------------------------------------------

void MV88X3120_enable_eee(struct bdx_priv *priv)
{
	DBG("EEE enabled=%d\n", priv->eee_enabled);
	if(priv->eee_enabled)
	{
		DBG("EEE MTU=%d\n", priv->ndev->mtu);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x809C, 0xFF1);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x809D, 0x0);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x809E, 0x4B);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x809F, 0x0);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x8C02, (priv->ndev->mtu * 2));
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x8C03, 0x0);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x8E02, (priv->ndev->mtu * 2));
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x8E03, 0x0);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x8000, 0x3);
		bdx_mdio_write(priv, 9, priv->phy_mdio_port, 0x8001, 0x7E50);
	}
	else
	{
#if defined(_EEE_) && defined(ETHTOOL_SEEE)
		MV88X3120_set_eee(priv/*, edata*/);
#endif
	}
} // MV88X3120_enable_eee()

//-------------------------------------------------------------------------------------------------

int MV88X3120_reset_eee(struct bdx_priv *priv)
{
   // struct bdx_priv *priv = netdev_priv(netdev);

	bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49230, 0);
	bdx_mdio_write(priv, 1, priv->phy_mdio_port, 49221, 0);
	bdx_mdio_write(priv, 7, priv->phy_mdio_port, 60, 0);
	priv->eee_enabled=false;

	return 0;

} // MV88X3120_reset_eee()

#endif
//-------------------------------------------------------------------------------------------------

__init void MV88X3120_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings = MV88X3120_get_settings;
    priv->phy_ops.set_settings = MV88X3120_set_settings;
#ifdef _EEE_
#ifdef ETHTOOL_GEEE
    priv->phy_ops.get_eee 	   = MV88X3120_get_eee;
#endif
#ifdef ETHTOOL_SEEE
    priv->phy_ops.set_eee 	   = MV88X3120_set_eee;
#endif
    priv->phy_ops.reset_eee    = MV88X3120_reset_eee;
#endif
} // MV88X3120_register_settings()

//-------------------------------------------------------------------------------------------------


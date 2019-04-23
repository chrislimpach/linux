#include "tn40.h"

int AQR105_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
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

} // AQR105_get_settings()

//-------------------------------------------------------------------------------------------------

int AQR105_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);
	u16 val, port=priv->phy_mdio_port;
	s32 speed=ethtool_cmd_speed(ecmd);

#if 0 // debug
		ERR("AQR105 ecmd->cmd=%x\n", ecmd->cmd);
		ERR("AQR105 speed=%u\n",speed);
		ERR("AQR105 ecmd->autoneg=%u\n",ecmd->autoneg);
#endif

	if(AUTONEG_ENABLE == ecmd->autoneg){
	DBG("AQR105 speed %d Autoneg\n", speed);
	BDX_MDIO_WRITE(priv, 0x07,0x10,0x9101);
	BDX_MDIO_WRITE(priv, 0x07,0xC400,0x9C5A);
	BDX_MDIO_WRITE(priv, 0x07,0x20,0x1001);
	priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_Autoneg | ADVERTISED_Pause);
	priv->autoneg     = AUTONEG_ENABLE;
	} else {
	priv->autoneg     = AUTONEG_DISABLE;
	switch(speed){
        case 10000: //10G
		DBG("AQR105 speed 10G\n");
		BDX_MDIO_WRITE(priv, 0x07,0x10,0x9001);
		BDX_MDIO_WRITE(priv, 0x07,0xC400,0x40);
		BDX_MDIO_WRITE(priv, 0x07,0x20,0x1001);
		priv->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_Pause);
        	break;
        case 5000: //5G
    	DBG("AQR105 speed 5G\n");
		BDX_MDIO_WRITE(priv, 0x07,0x10,0x9001);
		BDX_MDIO_WRITE(priv, 0x07,0xC400,0x840);
		BDX_MDIO_WRITE(priv, 0x07,0x20,0x1);
		priv->advertising = (/*ADVERTISED_5000baseT_Full | */ADVERTISED_Pause);
        	break;
        case 2500: //2.5G
    	DBG("AQR105 speed 10G\n");
		BDX_MDIO_WRITE(priv, 0x07,0x10,0x9001);
		BDX_MDIO_WRITE(priv, 0x07,0xC400,0x440);
		BDX_MDIO_WRITE(priv, 0x07,0x20,0x1);
		priv->advertising = (/*ADVERTISED_2500baseT_Full |*/ ADVERTISED_Pause);
        	break;
        case 1000:  //1G
    	DBG("AQR105 speed 1G\n");
		BDX_MDIO_WRITE(priv, 0x07,0x10,0x8001);
		BDX_MDIO_WRITE(priv, 0x07,0xC400,0x8040);
		BDX_MDIO_WRITE(priv, 0x07,0x20,0x1);
		priv->advertising = (ADVERTISED_1000baseT_Full | ADVERTISED_Pause);
        	break;
        case 100:   //100m
    	DBG("AQR105 speed 100m\n");
		BDX_MDIO_WRITE(priv, 0x07,0x10,0x101);
		BDX_MDIO_WRITE(priv, 0x07,0xC400,0x40);
		BDX_MDIO_WRITE(priv, 0x07,0x20,0x1);
		priv->advertising = (ADVERTISED_100baseT_Full | ADVERTISED_Pause);
        	break;
        default :
		ERR("%s does not support speed %u\n", priv->ndev->name, speed);
             return -EINVAL;
        }
	}
// restart autoneg
        val=(1<<12)|(1<<9)|(1<<13);
	BDX_MDIO_WRITE(priv, 0x07,0x0,val);

	return 0;

} // AQR105_set_settings()

//-------------------------------------------------------------------------------------------------

__init void AQR105_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings = AQR105_get_settings;
    priv->phy_ops.set_settings = AQR105_set_settings;

} // MV88X3120_register_settings()

//-------------------------------------------------------------------------------------------------


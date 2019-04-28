#include "tn40.h"

//-------------------------------------------------------------------------------------------------

int QT2025_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   	= ( SUPPORTED_10000baseT_Full | SUPPORTED_FIBRE | SUPPORTED_Pause);
	ecmd->advertising 	= (ADVERTISED_10000baseT_Full | ADVERTISED_Pause );
	if (READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT)
	{
		ecmd->speed     =  priv->link_speed;
	}
	else
	{
		ecmd->speed     = 0;
	}
	ecmd->duplex      	= DUPLEX_FULL;
	ecmd->port        	= PORT_FIBRE;
	ecmd->transceiver 	= XCVR_EXTERNAL;
	ecmd->autoneg     	= AUTONEG_DISABLE;

	return 0;

} // QT2025_get_settings()

int QT2025_set_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);
		ERR("%s Does not support ethtool -s option\n", priv->ndev->name);
             return -EPERM;

} // QT2025_set_settings()

//-------------------------------------------------------------------------------------------------

__init void QT2025_register_settings(struct bdx_priv *priv)
{
    priv->phy_ops.get_settings = QT2025_get_settings;
    priv->phy_ops.set_settings = QT2025_set_settings;

} // QT2025_register_settings()

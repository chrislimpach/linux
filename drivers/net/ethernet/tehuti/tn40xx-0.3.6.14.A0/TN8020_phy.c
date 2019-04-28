#include "tn40.h"
#include "TN8020_phy.h"

void bdx_speed_changed(struct bdx_priv *priv, u32 speed);

//-------------------------------------------------------------------------------------------------

__init int TN8020_mdio_reset(struct bdx_priv *priv, int port,  unsigned short phy)
{
    int i;
    int phy_id=0;
    u16 adHi,adLo;
    u32 v, pl;
    u32 TN8020_phy_firmware_size=sizeof(TN8020_phy_firmware)/sizeof(u32);
    void __iomem 	*regs=priv->pBdxRegs;

	BDX_MDIO_WRITE(priv, 1,0x0000,(1<<15));
	for(i=200; i; i--) // 1s loop (0.5 in DS)
	{
		msleep(5);
		v= bdx_mdio_read(priv, 1, port, 0);
		if(!(v&(1<<15))) break;
	}
	if(v&(1<<15))
	{
		ERR("PHY PMA in permanent reset\n");
	}
	BDX_MDIO_WRITE(priv, 31,0x0000,3); // set PHY reset & auto-increment
	for(i=0; i<TN8020_phy_firmware_size;)
	{
		// Set addr
		adHi=(u16) (TN8020_phy_firmware[i]>>16);
		adLo=(u16) TN8020_phy_firmware[i];
		BDX_MDIO_WRITE(priv, 31,0x0021,adHi); // addr HI
		BDX_MDIO_WRITE(priv, 31,0x0020,adLo); // addr LO
		// get part lenght
		i++;
		pl=TN8020_phy_firmware[i++];
		// write FW data
		for(; (pl--) && (i<TN8020_phy_firmware_size); i++)
		{
			adHi=(u16) (TN8020_phy_firmware[i]>>16);
			adLo=(u16) TN8020_phy_firmware[i];
			BDX_MDIO_WRITE(priv, 31,0x0030,adLo); // data LO
			BDX_MDIO_WRITE(priv, 31,0x0030,adHi); // data HI
		}
		if( (i==TN8020_phy_firmware_size) && (++pl) )
		{
			ERR("InvalidHPY FW format i=%x aH=%x aL=%x\n",i,adHi,adLo);
		}
	}
	BDX_MDIO_WRITE(priv, 31,0x0000,0); // set PHY reset off
	for(i=100; --i;)
	{
		msleep(10);
		phy_id= bdx_mdio_read(priv, 31, port, 0);
		if(!(phy_id&(1<<1))) break;
	}
	if(!i)
	{
		ERR("PHY CPU in permanent reset\n");
	}
	msleep(10);
	//         BDX_MDIO_WRITE(priv, 1,0,0x2040);       // write  1.0 0x2040  # Force 10G
	BDX_MDIO_WRITE(priv, 1,0,0x2000);       // write  1.0 0x2000  # Force 1G
	BDX_MDIO_WRITE(priv, 7,23,0x4000);     // 1G fd
	//         BDX_MDIO_WRITE(priv, 7,16,0x9101);     // 100M fd
	BDX_MDIO_WRITE(priv, 30,30,1);         // SGMII AN on
	BDX_MDIO_WRITE(priv, 1,0x9002,1);      // LS Alarm on

    return 0;

} // TN8020_mdio_reset

//-------------------------------------------------------------------------------------------------

static int TN8020_mac_link_test(struct bdx_priv *priv)
{
    u32 i,j,r6040;

    for(j = 0; j < 10; j++)
    {
        i = READ_REG(priv, 0x6040);
        udelay(5);
        i = READ_REG(priv, 0x6040);
        r6040 = i;
        i &= 3;
        if (!i)
        {
        	break;
        }
        udelay(5);
    }
    // ERR("TN8020_mac_link_test 0x6040=0x%x loops=%i \n",r6040,j);

    return (i&3) ? 0 : 1;

} // TN8020_mac_link_test()

//-------------------------------------------------------------------------------------------------

u32 TN8020_link_changed(struct bdx_priv *priv)
{
    u32 link;

	link  = bdx_mdio_read(priv->pBdxRegs,30,priv->phy_mdio_port,1);
	link &= (1<<3);
	if(link)
	{
		u32 speed=bdx_mdio_read(priv->pBdxRegs,30,priv->phy_mdio_port,29);
		switch(speed&3)
		{
			case 0:
				link=SPEED_10000;
				break;

			case 1:
				link=SPEED_1000;
				break;

			case 2:
				link=SPEED_100;
				break;

			case 3:
				link=0;
				break;
		}
	}
	if(link!=priv->link_speed)
	{
		bdx_speed_changed(priv,link);
	}
	WRITE_REG(priv, 0x5150,(1<<31)|1000000); // reset timer for ~1/5 sec
	//Xaui_F2
	if(link)
	{
		if(!TN8020_mac_link_test(priv))
		{
			// ERR("bdx_mac_link_test failed loop=%i\n",priv->link_loop_cnt);
			if(priv->link_loop_cnt++ > 40)
			{
				bdx_speed_changed(priv,0);
				priv->link_loop_cnt=0;
			}
			link=0;
		}
	}

	return link;

} // TN8020_link_changed()

//-------------------------------------------------------------------------------------------------

int TN8020_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
	struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   = (SUPPORTED_10000baseT_Full  | SUPPORTED_1000baseT_Full  | SUPPORTED_100baseT_Full  | SUPPORTED_Autoneg  | SUPPORTED_TP | SUPPORTED_Pause);
	ecmd->advertising = (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full | ADVERTISED_Autoneg | ADVERTISED_Pause);
	ecmd->speed       = priv->link_speed;
	ecmd->duplex      = DUPLEX_FULL;
	ecmd->port        = PORT_TP;
	ecmd->transceiver = XCVR_INTERNAL;
	ecmd->autoneg     = AUTONEG_ENABLE;
	ecmd->autoneg     = AUTONEG_ENABLE;
#if defined(ETH_TP_MDI_AUTO)
	ecmd->eth_tp_mdix = ETH_TP_MDI_AUTO;
#else
#if (!defined VM_KLNX)
	ecmd->eth_tp_mdix = ETH_TP_MDI | ETH_TP_MDI_X;
#endif
#endif
	return 0;

} // TN8020_get_settings()

//-------------------------------------------------------------------------------------------------

void TN8020_leds(struct bdx_priv *priv, enum PHY_LEDS_OP op)
{
	switch (op)
	{
		case PHY_LEDS_SAVE:
			priv->phy_ops.leds = bdx_mdio_read(priv->pBdxRegs, 30, priv->phy_mdio_port, 34);
			break;

		case PHY_LEDS_RESTORE:
			bdx_mdio_write(priv, 30, priv->phy_mdio_port, 34, priv->phy_ops.leds);
			break;

		case PHY_LEDS_ON:
			bdx_mdio_write(priv, 30, priv->phy_mdio_port, 34, 0x100);
			break;

		case PHY_LEDS_OFF:
			bdx_mdio_write(priv, 30, priv->phy_mdio_port, 34, 0x30);
			break;

		default:
			DBG("TN8020_leds() unknown op 0x%x\n", op);
			break;

	}

} // TN8020_leds()


//-------------------------------------------------------------------------------------------------

__init unsigned short TN8020_register(struct bdx_priv *priv)
{
    priv->isr_mask= IR_RX_FREE_0 | IR_LNKCHG1 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = TN8020_mdio_reset;
    priv->phy_ops.link_changed = TN8020_link_changed;
    priv->phy_ops.get_settings = TN8020_get_settings;
    priv->phy_ops.ledset       = TN8020_leds;

    return PHY_TYPE_TN8020;

} // TN8020_init()

//-------------------------------------------------------------------------------------------------


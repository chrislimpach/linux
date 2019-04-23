#include "tn40.h"
#include "QT2025_phy.h"

#define LINK_LOOP_MAX   (10)

void bdx_speed_changed(struct bdx_priv *priv, u32 speed);

//-------------------------------------------------------------------------------------------------

__init int QT2025_mdio_reset(void __iomem * regs, int port,  unsigned short phy)
{
    u16 *phy_fw= QT2025_phy_firmware,j, fwVer01, fwVer2, fwVer3;
    int s_fw, i, a;
    int phy_id= 0, rev= 0;


    port=bdx_mdio_look_for_phy(regs,port);
    if (port<0) return 0; // HPY not found

    phy_id= bdx_mdio_read(regs, 1, port, 0xD001);
    //  ERR("PHY_ID %x, port=%x\n", phy_id, port);

    switch(0xFF&(phy_id>> 8))
    {
        case 0xb3:
            rev= 0xd;
            s_fw= sizeof(QT2025_phy_firmware) / sizeof(u16);
            break;
        default:
            ERR("PHY ID =0x%x, returning\n", (0xFF & (phy_id >> 8)));
            return 1;
    }

    switch(rev)
    {
        default:
            ERR("bdx: failed unknown PHY ID %x\n", phy_id);
            \
            return 1;
        case 0xD:
            BDX_MDIO_WRITE(1,0xC300,0x0000);
            BDX_MDIO_WRITE(1,0xC302,0x4);
            BDX_MDIO_WRITE(1,0xC319,0x0038);
            //         BDX_MDIO_WRITE(1,0xC319,0x0088);  //1G
            BDX_MDIO_WRITE(1,0xC31A,0x0098);
            BDX_MDIO_WRITE(3,0x0026,0x0E00);
            //         BDX_MDIO_WRITE(3,0x0027,0x08983);  //10G
            BDX_MDIO_WRITE(3,0x0027,0x0893);  //10G
            //         BDX_MDIO_WRITE(3,0x0027,0x01092);    //1G     //3092
            BDX_MDIO_WRITE(3,0x0028,0xA528);
            BDX_MDIO_WRITE(3,0x0029,0x03);
            BDX_MDIO_WRITE(1,0xC30A,0x06E1);
            BDX_MDIO_WRITE(1,0xC300,0x0002);
            BDX_MDIO_WRITE(3,0xE854,0x00C0);

            /* Dump firmware starting at address 3.8000 */
            for (i = 0, j = 0x8000, a = 3; i < s_fw; i++, j++)
            {
                if(i == 0x4000)
                {
                    a = 4;
                    j = 0x8000;
                }
                if (phy_fw[i] < 0x100)
                    BDX_MDIO_WRITE(a, j, phy_fw[i]);
            }
            BDX_MDIO_WRITE(3, 0xE854, 0x0040);
            for(i = 60; i; i--)
            {
                msleep(50);
                j = bdx_mdio_read(regs, 3, port, 0xD7FD);
                if(!(j== 0x10 || j== 0))
                    break;
            }
            if(!i)  ERR("PHY init error\n");
            break;
    }
    fwVer01 = bdx_mdio_read(regs, 3, port, 0xD7F3);
    fwVer2  = bdx_mdio_read(regs, 3, port, 0xD7F4);
    fwVer3  = bdx_mdio_read(regs, 3, port, 0xD7F5);

    ERR("QT2025 FW version %d.%d.%d.%d",
    		((fwVer01 >> 4) & 0xf),
    		(fwVer01 & 0xf),
    		(fwVer2 & 0xff),
    		(fwVer3 & 0xff));

    return 0;

} // QT2025_mdio_reset

//-------------------------------------------------------------------------------------------------

u32 QT2025_link_changed(struct bdx_priv *priv)
{
     u32 link=0;

     if (priv->link_speed != SPEED_10000)
     {
    	 bdx_speed_changed(priv, SPEED_10000);
     }
     link = READ_REG(priv,regMAC_LNK_STAT) &  MAC_LINK_STAT;
     if (link)
     {
    	 link = SPEED_10000;
    	 DBG("QT2025 link speed is 10G\n");
     }
     else
     {
    	 if(priv->link_loop_cnt++ > LINK_LOOP_MAX)
    	 {
    		 DBG("QT2025 MAC reset\n");
    		 priv->link_speed    = 0;
    		 priv->link_loop_cnt = 0;
    	 }
    	 DBG("QT2025 no link, setting 1/5 sec timer\n");
    	 WRITE_REG(priv, 0x5150,1000000); // 1/5 sec timeout
     }

     return link;

} // QT2025_link_changed()

//-------------------------------------------------------------------------------------------------

int QT2025_get_settings(struct net_device *netdev, struct ethtool_cmd *ecmd)
{
    struct bdx_priv *priv = netdev_priv(netdev);

	ecmd->supported   	= (SUPPORTED_10000baseT_Full  | SUPPORTED_1000baseT_Full  | SUPPORTED_100baseT_Full | SUPPORTED_Autoneg  );
	ecmd->advertising 	= (ADVERTISED_10000baseT_Full | ADVERTISED_1000baseT_Full | ADVERTISED_100baseT_Full );
	ecmd->speed       	=  priv->link_speed; // SPEED_10000;
	ecmd->duplex      	= DUPLEX_FULL;
	ecmd->port        	= PORT_TP;
	ecmd->transceiver 	= XCVR_EXTERNAL;  // What does it mean?
	ecmd->autoneg     	= AUTONEG_ENABLE;

	return 0;

} // QT2025_get_settings()

//-------------------------------------------------------------------------------------------------

__init unsigned short QT2025_register(struct bdx_priv *priv)
{
	priv->isr_mask= IR_RX_FREE_0 |  IR_LNKCHG0 | IR_PSE | IR_TMR0 | IR_RX_DESC_0 | IR_TX_FREE_0;
    priv->phy_ops.mdio_reset   = QT2025_mdio_reset;
    priv->phy_ops.link_changed = QT2025_link_changed;
    priv->phy_ops.get_settings = QT2025_get_settings;

    return PHY_TYPE_QT2025;

} // QT2025_init()

//-------------------------------------------------------------------------------------------------


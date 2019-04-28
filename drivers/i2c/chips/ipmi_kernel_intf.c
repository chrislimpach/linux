#include <linux/ipmi.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/device.h>

#define IPMI_NET_FUNCTION	0x06
#define IPMI_COMMAND		0x52
#define IPMI_I2C_REG		0x0f
#define IPMI_WITHOUT_OUTPUT	0x00
#define IPMI_WITH_OUTPUT	0x01

/* CPU FAN */
#define CPU_FAN_READ_NETFN	0x04
#define CPU_FAN_READ_CMD	0x2d
#define CPU_FAN_READ_DATA1	0x0f
#define CPU_FAN_WRITE_NETFN	0x3a
#define CPU_FAN_WRITE_CMD	0x01

/* Temperature */
#define TEMP_NETFN		0x04
#define TEMP_CMD		0x2d
#define TEMP_CPU_DATA		0x23
#define TEMP_MB_DATA		0x21
#define TEMP_TR1_DATA		0x22

/* F75387 */
#define f75387_CHIPID_REG	0x5a
#define f75387_FAN_MODE		0x60

#define COUNT2RPM(i) (1500000/i)

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static void ipmi_register_dev(int intf_num, struct device *dev);
static void ipmi_dev_gone(int iface);

struct ipmi_gpio_data {
	struct list_head	list;
	struct ipmi_addr	address;
	int			interface;
	struct device		*ipmi_device;
	struct completion	read_complete;
	struct mutex		lock;
	ipmi_user_t		user;

	struct kernel_ipmi_msg	tx_message;
	long			tx_msgid;
	unsigned char		tx_msg_data[IPMI_MAX_MSG_LENGTH];

	unsigned char		rx_msg_data[IPMI_MAX_MSG_LENGTH];
	int			rx_recv_type;
	unsigned char		rx_result;
	unsigned long		rx_msg_len;
	
};

struct ipmi_driver_data {
	struct list_head	ipmi_driver_data;
	struct ipmi_smi_watcher ipmi_driver_events;
	struct ipmi_user_hndl	ipmi_hndlrs;
};

static struct ipmi_driver_data driver_data = {
	.ipmi_driver_data = LIST_HEAD_INIT(driver_data.ipmi_driver_data),
	.ipmi_driver_events = {
		.owner		= THIS_MODULE,
		.new_smi	= ipmi_register_dev,
		.smi_gone	= ipmi_dev_gone,
	},
	.ipmi_hndlrs = {
		.ipmi_recv_hndl	= ipmi_msg_handler,
	},
};

static struct ipmi_gpio_data *ipmi_dev_data = NULL;

static int ipmi_send_message(struct ipmi_gpio_data *data)
{
	int err;

	err = ipmi_validate_addr(&data->address, sizeof(data->address));
	if (err)
		goto out;

	err = ipmi_request_settime(data->user, &data->address, data->tx_msgid,
			 &data->tx_message, NULL, 0, 0, 0);
	if (err)
		goto out1;

	return 0;

out1:
	printk(KERN_ERR "ipmi_kernel_intf: Invalid IPMI Request(%x)\n", err);
	return err;
out:
	printk(KERN_ERR "ipmi_kernel_intf: Invalid Validate Address(%x)\n", err);
	return err;
}

int ipmi_cpu_fan_read(void)
{
	int ret;
	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;
	/* Prepare Data */
	ipmi_dev_data->tx_msg_data[0] = CPU_FAN_READ_DATA1;
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = CPU_FAN_READ_NETFN;
	ipmi_dev_data->tx_message.cmd = CPU_FAN_READ_CMD;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 1;

	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return (int) ipmi_dev_data->rx_msg_data[0];
}
EXPORT_SYMBOL(ipmi_cpu_fan_read);

int ipmi_cpu_fan_write(unsigned long val)
{
	int ret;

	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	/* val=0 -> Smart FAN Mode; val=01h~64h -> Manual FAN Mode*/
	ipmi_dev_data->tx_msg_data[0] = val; // CPU1
	ipmi_dev_data->tx_msg_data[1] = 0x0; // CPU2
	ipmi_dev_data->tx_msg_data[2] = 0x0; // REAR1
	ipmi_dev_data->tx_msg_data[3] = 0x0; // REAR2
	ipmi_dev_data->tx_msg_data[4] = 0x0; // FRONT1
	ipmi_dev_data->tx_msg_data[5] = 0x0; // FRONT2
	ipmi_dev_data->tx_msg_data[6] = 0x0; // FRONT3
	ipmi_dev_data->tx_msg_data[7] = 0x0; // FRONT4
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = CPU_FAN_WRITE_NETFN;
	ipmi_dev_data->tx_message.cmd = CPU_FAN_WRITE_CMD;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 8;

	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return ret;
}
EXPORT_SYMBOL(ipmi_cpu_fan_write);

/*val=0 -> CPU TEMP; val=1 -> M/B TEMP; val=2 -> TR1 TEMP */
int ipmi_temp_read(int val)
{
	int ret;

	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	switch(val){
	case 0: 
		ipmi_dev_data->tx_msg_data[0] = TEMP_CPU_DATA;
		break;
	case 1:
		ipmi_dev_data->tx_msg_data[0] = TEMP_MB_DATA;
		break;
	case 2:
		ipmi_dev_data->tx_msg_data[0] = TEMP_TR1_DATA;
		break;
	}
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = TEMP_NETFN;
	ipmi_dev_data->tx_message.cmd = TEMP_CMD;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 1;

	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return (int) ipmi_dev_data->rx_msg_data[0];
}

int ipmi_cpu_temp_read(void)
{
	return ipmi_temp_read(0);
}
EXPORT_SYMBOL(ipmi_cpu_temp_read);

int ipmi_mb_temp_read(void)
{
	return ipmi_temp_read(1);
}
EXPORT_SYMBOL(ipmi_mb_temp_read);

int ipmi_tr1_temp_read(void)
{
	return ipmi_temp_read(2);
}
EXPORT_SYMBOL(ipmi_tr1_temp_read);

unsigned long f75387_ipmi_read(u8 dev_id, u8 reg)
{
	int ret;

	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	ipmi_dev_data->tx_msg_data[0] = IPMI_I2C_REG;
	ipmi_dev_data->tx_msg_data[1] = dev_id;
	ipmi_dev_data->tx_msg_data[2] = IPMI_WITH_OUTPUT;
	ipmi_dev_data->tx_msg_data[3] = reg;
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = IPMI_NET_FUNCTION;
	ipmi_dev_data->tx_message.cmd = IPMI_COMMAND;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 4;
	
	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return ipmi_dev_data->rx_msg_data[0];
}

int f75387_ipmi_write(u8 dev_id, u8 reg, u8 val)
{
	int ret;

	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	ipmi_dev_data->tx_msg_data[0] = IPMI_I2C_REG;
	ipmi_dev_data->tx_msg_data[1] = dev_id;
	ipmi_dev_data->tx_msg_data[2] = IPMI_WITHOUT_OUTPUT;
	ipmi_dev_data->tx_msg_data[3] = reg;
	ipmi_dev_data->tx_msg_data[4] = val;
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = IPMI_NET_FUNCTION;
	ipmi_dev_data->tx_message.cmd = IPMI_COMMAND;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 5;

	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return ret;
}
EXPORT_SYMBOL(f75387_ipmi_write);

u16 f75387_ipmi_read_chip_id(u8 dev_id)
{
	u8 high_byte, low_byte;

	high_byte = f75387_ipmi_read(dev_id, f75387_CHIPID_REG);
	low_byte = f75387_ipmi_read(dev_id, f75387_CHIPID_REG+1);

	return (high_byte<<8) | low_byte;
}
EXPORT_SYMBOL(f75387_ipmi_read_chip_id);

int f75387_ipmi_read_hdd_fan(u8 dev_id, u8 reg){
	u8 fan_msb, fan_lsb;
	u16 ret;

	fan_msb = f75387_ipmi_read(dev_id, reg);
	fan_lsb = f75387_ipmi_read(dev_id, reg+1);
	ret = (fan_msb<<8) | fan_lsb;

	if (ret == 0xfff || ret == 0xffe)
		return 0;
	else
		return COUNT2RPM((int) ret);
}
EXPORT_SYMBOL(f75387_ipmi_read_hdd_fan);

void f75387_hddfan_initial(u8 dev_id)
{
	u8 val;
	
	/* Set to manual mode. */
	val = f75387_ipmi_read(dev_id, f75387_FAN_MODE);
	f75387_ipmi_write(dev_id, f75387_FAN_MODE, (val | 0x55));

	/* Set FAN2 PIN4_MODE to PWMOUT2 and PIN2_MODE to FANIN2 input. */
	val = f75387_ipmi_read(dev_id, 0x01);
	f75387_ipmi_write(dev_id, 0x01, (val | 0x03));
}
EXPORT_SYMBOL(f75387_hddfan_initial);

int f75387_temp_read(u8 dev_id, u8 reg)
{
	return (int) f75387_ipmi_read(dev_id, reg);
}
EXPORT_SYMBOL(f75387_temp_read);

unsigned long pca9532_input_ipmi_read(unsigned long pca9532_dev_reg, int read_data_reg)
{
	int ret;

	if(!ipmi_dev_data){
		printk("IPMI Data has not initialized yet!\n");
		return -ENODEV;
	}

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	ipmi_dev_data->tx_msg_data[0] = IPMI_I2C_REG;
	ipmi_dev_data->tx_msg_data[1] = pca9532_dev_reg;
	ipmi_dev_data->tx_msg_data[2] = IPMI_WITH_OUTPUT;
	ipmi_dev_data->tx_msg_data[3] = read_data_reg;
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = IPMI_NET_FUNCTION;
	ipmi_dev_data->tx_message.cmd = IPMI_COMMAND;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 4;
	
	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	if (ipmi_dev_data->rx_result){
		return -ENOENT;
        } else {
		return ipmi_dev_data->rx_msg_data[0];
        }
}
EXPORT_SYMBOL(pca9532_input_ipmi_read);

unsigned long pca9532_ipmi_read(unsigned long pca9532_dev_reg, unsigned long read_data_reg)
{
	int ret;

	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	ipmi_dev_data->tx_msg_data[0] = IPMI_I2C_REG;
	ipmi_dev_data->tx_msg_data[1] = pca9532_dev_reg;
	ipmi_dev_data->tx_msg_data[2] = IPMI_WITH_OUTPUT;
	ipmi_dev_data->tx_msg_data[3] = read_data_reg;
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = IPMI_NET_FUNCTION;
	ipmi_dev_data->tx_message.cmd = IPMI_COMMAND;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 4;
	
	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return ipmi_dev_data->rx_msg_data[0];
}
EXPORT_SYMBOL(pca9532_ipmi_read);

int pca9532_ipmi_write(unsigned long pca9532_dev_reg, unsigned long access_reg,
			unsigned long access_val)
{
	int ret;

	if(!ipmi_dev_data)
		printk("IPMI Data has not initialized yet!\n");

	mutex_lock(&ipmi_dev_data->lock);
	ipmi_dev_data->tx_message.data = kmalloc(IPMI_MAX_MSG_LENGTH, GFP_KERNEL);
	if (!ipmi_dev_data->tx_message.data)
		return -ENOMEM;

	/* Prepare Data */
	ipmi_dev_data->tx_msg_data[0] = IPMI_I2C_REG;
	ipmi_dev_data->tx_msg_data[1] = pca9532_dev_reg;
	ipmi_dev_data->tx_msg_data[2] = IPMI_WITHOUT_OUTPUT;
	ipmi_dev_data->tx_msg_data[3] = access_reg;
	ipmi_dev_data->tx_msg_data[4] = access_val;
	/* Setup the transfer data */
	ipmi_dev_data->tx_message.netfn = IPMI_NET_FUNCTION;
	ipmi_dev_data->tx_message.cmd = IPMI_COMMAND;
	ipmi_dev_data->tx_message.data = ipmi_dev_data->tx_msg_data;
	ipmi_dev_data->tx_message.data_len = 5;

	ret = ipmi_send_message(ipmi_dev_data);
	wait_for_completion(&ipmi_dev_data->read_complete);

	mutex_unlock(&ipmi_dev_data->lock);
	return ret;
}
EXPORT_SYMBOL(pca9532_ipmi_write);

static void ipmi_register_dev(int intf_num, struct device *dev)
{
	int err;

	ipmi_dev_data = kmalloc(sizeof(*ipmi_dev_data), GFP_KERNEL);
	if(!ipmi_dev_data) {
		printk(KERN_ERR "ipmi_kernerl_intf: Insufficient memory for ipmi interface\n");
		return;
	}

	ipmi_dev_data->address.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
	ipmi_dev_data->address.channel = IPMI_BMC_CHANNEL;
	ipmi_dev_data->address.data[0] = 0;
	ipmi_dev_data->interface = intf_num;
	ipmi_dev_data->ipmi_device = dev;

	/* Create IPMI messaging interface user */
	err = ipmi_create_user(ipmi_dev_data->interface, &driver_data.ipmi_hndlrs,
				ipmi_dev_data, &ipmi_dev_data->user);
	if (err < 0){
		printk(KERN_ERR "ipmi_kerenl_intf: Unable to register user with IPMI interface %d\n", ipmi_dev_data->interface);
		goto out;
	}

	mutex_init(&ipmi_dev_data->lock);

	/* Initialize message */
	ipmi_dev_data->tx_msgid = 0;
	init_completion(&ipmi_dev_data->read_complete);

	/* Add the new device data to the data list*/
	dev_set_drvdata(dev, ipmi_dev_data);
	list_add_tail(&ipmi_dev_data->list, &driver_data.ipmi_driver_data);

	return;
out:
	kfree(ipmi_dev_data);
}

static void ipmi_dev_delete(struct ipmi_gpio_data *data)
{
	list_del(&data->list);
	dev_set_drvdata(data->ipmi_device, NULL);
	ipmi_destroy_user(data->user);
	kfree(data);
}

static void ipmi_dev_gone(int iface)
{
	if (ipmi_dev_data->interface == iface)
		ipmi_dev_delete(ipmi_dev_data);
}

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data)
{
	struct ipmi_gpio_data *data = (struct ipmi_gpio_data *)user_msg_data;

	if (msg->msgid != data->tx_msgid) {
		printk(KERN_ERR "Mismatch between received msgid (%02x) and transmitted msgid (%02x)!\n",
		(int)msg->msgid,
		(int)data->tx_msgid);
		ipmi_free_recv_msg(msg);
		return;
	}
	
	data->rx_recv_type = msg->recv_type;
	if (msg->msg.data_len > 0){
		data->rx_result = msg->msg.data[0];
		ipmi_dev_data->rx_result = data->rx_result;
	}else
		ipmi_dev_data->rx_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;

	if (msg->msg.data_len > 1){
		data->rx_msg_len = msg->msg.data_len - 1;
		memcpy(data->rx_msg_data, msg->msg.data + 1, data->rx_msg_len);
	} else
		data->rx_msg_len = 0;

	ipmi_free_recv_msg(msg);
	complete(&data->read_complete);
}

static int __init ipmi_intf_init(void)
{
	return ipmi_smi_watcher_register(&driver_data.ipmi_driver_events);
}

static void __exit ipmi_intf_exit(void)
{
	struct ipmi_gpio_data *p, *next;

	ipmi_smi_watcher_unregister(&driver_data.ipmi_driver_events);
	list_for_each_entry_safe(p, next, &driver_data.ipmi_driver_data, list)
		ipmi_dev_delete(p);
}

module_init(ipmi_intf_init);
module_exit(ipmi_intf_exit);

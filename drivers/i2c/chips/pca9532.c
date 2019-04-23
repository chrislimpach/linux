/*
 *  Copyright (C) 2010 Thecus Technology Corp. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for pca9532 chip on Thecus N16000
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

//#define DEBUG
#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (debug>=x) printk(KERN_DEBUG"%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

MODULE_AUTHOR("Maintainer: Citizen Lee <citizen_lee@thecus.com>");
MODULE_DESCRIPTION("Thecus N16000 board (pca9532) Driver");
MODULE_LICENSE("GPL");
static int debug = 2;
module_param(debug, int, S_IRUGO | S_IWUSR);

extern unsigned long pca9532_ipmi_read(unsigned long pca9532_dev_reg, unsigned long read_data_reg);
extern int pca9532_ipmi_write(unsigned long pca9532_dev_reg, unsigned long access_reg, unsigned long access_val);
extern int has_ipmi_intf;

/* Addresses to scan */
static const unsigned short normal_i2c[] = {
    0x64, I2C_CLIENT_END
};

// i: 0, 1
#define PCA9532_REG_PSC(i) (0x2+(i)*2)
#define PCA9532_REG_PWM(i) (0x3+(i)*2)
#define PCA9532_REG_LS0  0x6
// led: 0 ~ 15
#define LED_REG(led) (((led&0xF)>>2)+PCA9532_REG_LS0)
#define LED_NUM(led) (led & 0x3)

#define LED_PSC0 0x2
#define LED_PWM0 0x3
#define LED_PSC1 0x4
#define LED_PWM1 0x5

#define LED_OFF 0x0
#define LED_ON 0x1
#define LED_BLINK1 0x2
#define LED_BLINK2 0x3

#define PCA9532_IPMI_DEV_ID 0xc8

struct pca9532_data {
    struct i2c_client *client;
    struct mutex update_lock;
};

static int pca9532_probe(struct i2c_client *client,
			 const struct i2c_device_id *id);
static int pca9532_remove(struct i2c_client *client);

static const struct i2c_device_id pca9532_id[] = {
    {"pca9532", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, pca9532_id);

static struct i2c_client *pca9532_client = NULL;

/* Return 0 if detection is successful, -ENODEV otherwise */
static int pca9532_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    unsigned short address = client->addr;
    const char *chip_name = "pca9532";

    strlcpy(info->type, chip_name, I2C_NAME_SIZE);
    dev_info(&adapter->dev, "Found %s at 0x%02hx\n", chip_name, address);

    return 0;
}

/* Set DiSK Error LED via IPMI. led_num: 0~15, led_state: 0, 1, 2 */
void pca9532_ipmi_set_led(u8 dev_id, int led_num, int led_state)
{
	/* Get the current led group's value */
	u8 led_val = pca9532_ipmi_read(dev_id, LED_REG(led_num));
	/* Caculate the setting value */
	led_val = led_val & ~(0x3 << LED_NUM(led_num) * 2);
	led_val = led_val | (led_state << LED_NUM(led_num) * 2);

	pca9532_ipmi_write(dev_id, LED_REG(led_num), led_val);
}
EXPORT_SYMBOL(pca9532_ipmi_set_led);

/* Set Disk Eroor LED via i2C. led_num: 0 ~ 15, led_state: 0, 1, 2 */
void pca9532_set_led(int led_num, int led_state)
{
    int reg;
    struct i2c_client *client = pca9532_client;
    struct pca9532_data *data = NULL;

    if ((NULL == client) && (has_ipmi_intf == 0)) {
	printk(KERN_INFO "pca9532_set_led: Can't get i2c client or IPMI interface\n");
	return;
    }

    /* Get the current value */
    if (has_ipmi_intf == 1){
        reg = pca9532_ipmi_read(PCA9532_IPMI_DEV_ID, LED_REG(led_num));
    } else {
        data = i2c_get_clientdata(client);
        mutex_lock(&data->update_lock);
        reg = i2c_smbus_read_byte_data(client, LED_REG(led_num));
    }

    /* zero led bits */
    reg = reg & ~(0x3 << LED_NUM(led_num) * 2);
    /* set the new value */
    reg = reg | (led_state << LED_NUM(led_num) * 2);

    /* Set value */
    if (has_ipmi_intf == 1) {
        pca9532_ipmi_write(PCA9532_IPMI_DEV_ID, LED_REG(led_num), reg);
    } else {
        i2c_smbus_write_byte_data(client, LED_REG(led_num), reg);
        mutex_unlock(&data->update_lock);
    }
}
EXPORT_SYMBOL(pca9532_set_led);

/* return 0 for no error */
static int
pca9532_rw(struct i2c_client *client, u8 reg_num, u8 * val, int wr)
{
    int ret;
    struct pca9532_data *data = NULL;

    if ((NULL == client) && (has_ipmi_intf == 0)) {
	printk(KERN_INFO "pca9532_rw: Can't get i2c client or IPMI interface\n");
	return 1;
    }
    //_DBG(1, "pca9532_rw reg: %d, value: %d, %s\n", reg_num, *val,
    // (wr > 0) ? "write" : "read");
    if (wr) {			//Write
        if (has_ipmi_intf == 1) {
            ret = pca9532_ipmi_write(PCA9532_IPMI_DEV_ID, reg_num, *val);
        } else {
	    data = i2c_get_clientdata(client); 
	    mutex_lock(&data->update_lock);
	    ret = i2c_smbus_write_byte_data(client, reg_num, *val);
	    mutex_unlock(&data->update_lock);
        }
	if (ret != 0) {
	    printk(KERN_INFO "pca9532_rw: cant write PCA9532 Reg#%02X\n",
		   reg_num);
	    return ret;
	}
    } else {			//Read
        if (has_ipmi_intf == 1) {
            ret = pca9532_ipmi_read(PCA9532_IPMI_DEV_ID, reg_num);
        } else {
	    mutex_lock(&data->update_lock);
            ret = i2c_smbus_read_byte_data(client, reg_num);
	    mutex_unlock(&data->update_lock);
        }
	if (ret < 0) {
	    printk(KERN_INFO "pca9532_rw: cant read PCA9532 Reg#%02X\n",
		   reg_num);
	    return ret;
	}
	*val = ret;
    }
    return 0;
}

static int pca9532_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
    struct pca9532_data *data;
    u8 val2 = 0;
    int i = 0;

    /* Only for i2c device */
    if (has_ipmi_intf == 0) {
        if (!i2c_check_functionality(client->adapter,
	        			 I2C_FUNC_SMBUS_BYTE_DATA))
	    return -EIO;

        data = kzalloc(sizeof(*data), GFP_KERNEL);
        if (!data)
	    return -ENOMEM;

        dev_info(&client->dev, "setting platform data\n");
        data->client = client;
        i2c_set_clientdata(client, data);
        mutex_init(&data->update_lock);
        pca9532_client = client;
    }

    // initial value, led blink frequency
    // echo "Freq 1 3" > /proc/thecus_io
    val2 = (152 / 3) - 1;
    pca9532_rw(pca9532_client, PCA9532_REG_PSC(0), &val2, 1);

    // turn off all led
    for (i = 0; i < 16; i++)
	pca9532_set_led(i, LED_OFF);

    return 0;
}

static int pca9532_remove(struct i2c_client *client)
{
    struct pca9532_data *data = i2c_get_clientdata(client);
    kfree(data);
    pca9532_client = NULL;
    i2c_set_clientdata(client, NULL);
    return 0;
}

static ssize_t
proc_pca9532_write(struct file *file, const char __user * buf,
		   size_t length, loff_t * ppos)
{
    char *buffer;
    int i, err, val, v1, v2;
    u8 val1, val2;

    if (!buf || length > PAGE_SIZE)
	return -EINVAL;

    buffer = (char *) __get_free_page(GFP_KERNEL);
    if (!buffer)
	return -ENOMEM;

    err = -EFAULT;
    if (copy_from_user(buffer, buf, length))
	goto out;

    err = -EINVAL;
    if (length < PAGE_SIZE)
	buffer[length] = '\0';
    else if (buffer[PAGE_SIZE - 1])
	goto out;

    /*
     * Usage: echo "S_LED 1-16 0|1|2" >/proc/pca9532 //2:Blink
     * Usage: echo "Freq 1-2 1-152" > /proc/pca9532
     * Usage: echo "Duty 1-2 1-256" > /proc/pca9532
     * 
     */
    if (!strncmp(buffer, "S_LED", strlen("S_LED"))) {
	i = sscanf(buffer + strlen("S_LED"), "%d %d\n", &v1, &v2);
	if (i == 2)		//two input
	{
	    if (v2 == 0)	//input 0: want to turn off
		val = LED_OFF;
	    else if (v2 == 1)	//turn on
		val = LED_ON;
	    else
		val = LED_BLINK1;

	    if (v1 >= 1 && v1 <= 16) {
		v1 = v1 - 1;
		pca9532_set_led(v1, val);
	    }
	}
    } else if (!strncmp(buffer, "Freq", strlen("Freq"))) {
	i = sscanf(buffer + strlen("Freq"), "%d %d\n", &v1, &v2);
	if (i == 2)		//two input
	{
	    if (v1 == 1)	//input 1: PSC0
		val = LED_PSC0;
	    else
		val = LED_PSC1;

	    val2 = (152 / v2) - 1;
	    _DBG(1, "port=0x%02X,val=0x%02X\n", val, val2);
	    pca9532_rw(pca9532_client, val, &val2, 1);
	}
    } else if (!strncmp(buffer, "Duty", strlen("Duty"))) {
	i = sscanf(buffer + strlen("Duty"), "%d %d\n", &v1, &v2);
	if (i == 2)		//two input
	{
	    if (v1 == 1)	//input 1: PWM0
		val1 = LED_PWM0;
	    else
		val1 = LED_PWM1;

	    val2 = (v2 * 256) / 100;
	    pca9532_rw(pca9532_client, val1, &val2, 1);
	}
    } else;

    err = length;
  out:
    free_page((unsigned long) buffer);
    *ppos = 0;

    return err;
}

static int proc_pca9532_show(struct seq_file *m, void *v)
{
    int i;
    u8 val1, val2;
    char LED_STATUS[4][8];

    sprintf(LED_STATUS[LED_ON], "ON");
    sprintf(LED_STATUS[LED_OFF], "OFF");
    sprintf(LED_STATUS[LED_BLINK1], "BLINK");
    sprintf(LED_STATUS[LED_BLINK2], "-");

    if (pca9532_client) {
	int j = 0;
	for (j = 0; j < 4; j++) {
	    if (!pca9532_rw(pca9532_client, PCA9532_REG_LS0 + j, &val1, 0)) {
		for (i = 0; i < 4; i++) {
		    val2 = (val1 >> (i * 2)) & 0x3;
		    seq_printf(m, "S_LED#%d: %s\n", j * 4 + (i + 1),
			       LED_STATUS[val2]);
		}
	    }
	}
    }
    return 0;
}

static int proc_pca9532_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_pca9532_show, NULL);
}

static struct file_operations proc_pca9532_operations = {
    .open = proc_pca9532_open,
    .read = seq_read,
    .write = proc_pca9532_write,
    .llseek = seq_lseek,
    .release = single_release,
};

int pca9532_init_procfs(void)
{
    struct proc_dir_entry *pde;

    pde = proc_create("pca9532", 0, NULL, &proc_pca9532_operations);
    if (pde == NULL) {
            return -ENOMEM;
    }

    return 0;
}

void pca9532_exit_procfs(void)
{
    remove_proc_entry("pca9532", NULL);
}

static struct i2c_driver pca9532_driver = {
    .driver = {
	       .name = "pca9532",
	       },
    .probe = pca9532_probe,
    .remove = pca9532_remove,
    .id_table = pca9532_id,

    .class = I2C_CLASS_HWMON,
    .detect = pca9532_detect,
    .address_list = normal_i2c,
};

static int __init pca9532_init(void)
{
    u8 val = 0;
    int i;

    if (pca9532_init_procfs()) {
	printk(KERN_ERR "pca9532: cannot create /proc/pca9532.\n");
	return -ENOENT;
    }

    printk("pca9532_init\n");
    /* If system uses IPMI to manage those sensors except i2C, 
     * it should set the default value in pca5932_init, because
     * pca9532_probe event is not triggered. */
    if (has_ipmi_intf == 1) {
        /* Initialize the value of LED Frequency */
        val = (152 / 3) - 1;
	pca9532_rw(NULL, PCA9532_REG_PSC(0), &val, 1);
	/* Turn off all LED */
	for (i = 0; i < 16; i++)
            pca9532_set_led(i, LED_OFF);
    }

    return (i2c_add_driver(&pca9532_driver));
}

static void __exit pca9532_exit(void)
{
    pca9532_exit_procfs();
    i2c_del_driver(&pca9532_driver);
}

module_init(pca9532_init);
module_exit(pca9532_exit);

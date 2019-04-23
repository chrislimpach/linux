/*
 *  Copyright (C) 2010 Thecus Technology Corp. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for pca9532 chip on Thecus
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
MODULE_DESCRIPTION("Thecus N8900 board (pca9532) Driver");
MODULE_LICENSE("GPL");
static int debug = 2;
module_param(debug, int, S_IRUGO | S_IWUSR);

/* Addresses to scan */
static const unsigned short normal_i2c[] = {
    0x61, I2C_CLIENT_END
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

#define PCA9532_OD_IPMI_DEV_ID 0xc2

extern unsigned long pca9532_ipmi_read(unsigned long pca9532_dev_reg, unsigned long read_data_reg);
extern int pca9532_ipmi_write(unsigned long pca9532_dev_reg, unsigned long access_reg, unsigned long access_val);
extern int has_ipmi_intf;

struct pca9532_od_data {
    struct i2c_client *client;
    struct mutex update_lock;
};

static int pca9532_od_probe(struct i2c_client *client,
			 const struct i2c_device_id *id);
static int pca9532_od_remove(struct i2c_client *client);

static const struct i2c_device_id pca9532_od_id[] = {
    {"pca9532_od", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, pca9532_od_id);

static struct i2c_client *pca9532_od_client = NULL;

/* Return 0 if detection is successful, -ENODEV otherwise */
static int pca9532_od_detect(struct i2c_client *client,
			  struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    unsigned short address = client->addr;
    const char *chip_name = "pca9532_od";

    strlcpy(info->type, chip_name, I2C_NAME_SIZE);
    dev_info(&adapter->dev, "Found %s at 0x%02hx\n", chip_name, address);

    return 0;
}

// led_num: 0 ~ 15, led_state: 0, 1, 2
void pca9532_od_set_led(int led_num, int led_state)
{
    int reg;
    struct i2c_client *client = pca9532_od_client;
    struct pca9532_od_data *data = NULL;

    if ((NULL == client) && (has_ipmi_intf == 0)) {
	printk(KERN_INFO "pca9532_od_set_led: i2c_client is NULL\n");
	return;
    }

    /* Get the current value */
    if (has_ipmi_intf == 1){
        reg = pca9532_ipmi_read(PCA9532_OD_IPMI_DEV_ID, LED_REG(led_num));
    } else {
        data = i2c_get_clientdata(client);
        mutex_lock(&data->update_lock);
        reg = i2c_smbus_read_byte_data(client, LED_REG(led_num));
    }
    /* zero led bits */
    reg = reg & ~(0x3 << (LED_NUM(led_num) * 2));
    /* set the new value */
    reg = reg | (led_state << (LED_NUM(led_num) * 2));

    /* Set value */
    if (has_ipmi_intf == 1) {
        pca9532_ipmi_write(PCA9532_OD_IPMI_DEV_ID, LED_REG(led_num), reg);
    } else {
        i2c_smbus_write_byte_data(client, LED_REG(led_num), reg);
        mutex_unlock(&data->update_lock);
    }
}
EXPORT_SYMBOL(pca9532_od_set_led);

// led_num: 0 ~ 15, return: 0, 1, 2
int pca9532_od_get_led(int led_num)
{
    int reg;
    struct i2c_client *client = pca9532_od_client;
    struct pca9532_od_data *data = NULL;

    if (NULL == client) {
	printk(KERN_INFO "pca9532_od_get_led: i2c_client is NULL\n");
	return 0;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    reg = i2c_smbus_read_byte_data(client, LED_REG(led_num));
    mutex_unlock(&data->update_lock);
    reg = 0x3 & (reg >> (LED_NUM(led_num) * 2));
    return reg;
}
EXPORT_SYMBOL(pca9532_od_get_led);

// led_num: 0 ~ 15, return: 0, 1
int pca9532_od_get_input(int led_num)
{
    int reg;
    struct i2c_client *client = pca9532_od_client;
    struct pca9532_od_data *data = NULL;
    led_num = led_num & 0xF;

    if (NULL == client) {
	printk(KERN_INFO "pca9532_od_get_led: i2c_client is NULL\n");
	return 0;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    if(led_num > 7 && led_num < 16)
        reg = i2c_smbus_read_byte_data(client, 1);
    else
        reg = i2c_smbus_read_byte_data(client, 0);
    mutex_unlock(&data->update_lock);
    if(led_num > 7 && led_num < 16) led_num -=8;
    reg = reg & (1 << led_num);

    return reg > 0 ? 1 : 0;
}
EXPORT_SYMBOL(pca9532_od_get_input);

/* return 0 for no error */
static int
pca9532_od_rw(struct i2c_client *client, u8 reg_num, u8 * val, int wr)
{
    int ret;
    struct pca9532_od_data *data = i2c_get_clientdata(client);

    if (NULL == client) {
	printk(KERN_INFO "pca9532_od_rw: i2c_client is NULL\n");
	return 1;
    }
    //_DBG(1, "pca9532_od_rw reg: %d, value: %d, %s\n", reg_num, *val,
    // (wr > 0) ? "write" : "read");
    if (wr) {			//Write
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_write_byte_data(client, reg_num, *val);
	mutex_unlock(&data->update_lock);
	if (ret != 0) {
	    printk(KERN_INFO "pca9532_od_rw: cant write PCA9532 Reg#%02X\n",
		   reg_num);
	    return ret;
	}
    } else {			//Read
	mutex_lock(&data->update_lock);
	ret = i2c_smbus_read_byte_data(client, reg_num);
	mutex_unlock(&data->update_lock);
	if (ret < 0) {
	    printk(KERN_INFO "pca9532_od_rw: cant read PCA9532 Reg#%02X\n",
		   reg_num);
	    return ret;
	}
	*val = ret;
    }
    return 0;
}

static int pca9532_od_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct pca9532_od_data *data;
    int i;
    u8 val1=0, val2=0;

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

    /* Check pca9532 function */
    for (i = 0; i < 4; i++) {
      if (pca9532_od_rw(client, PCA9532_REG_LS0 + i, &val1, 0)) {
        printk(KERN_INFO "pca9532_od: Remove pca9532_od driver\n");
        kfree(data);
        i2c_set_clientdata(client, NULL);
        remove_proc_entry("pca9532_od", NULL);
        printk(KERN_INFO "pca9532_od: Not support.\n");
        return -EIO;
      }
    }

    pca9532_od_client = client;

    /* initial value, led blink frequency
       echo "Freq 1 3" > /proc/thecus_io
    */
    val2 = (152 / 3) - 1;
    pca9532_od_rw(pca9532_od_client, PCA9532_REG_PSC(0), &val2, 1);

    /* turn off all led */
    for (i = 0; i < 16; i++)
      pca9532_od_set_led(i, LED_OFF);

      return 0;
}

static int pca9532_od_remove(struct i2c_client *client)
{
    struct pca9532_od_data *data = i2c_get_clientdata(client);
    kfree(data);
    pca9532_od_client = NULL;
    i2c_set_clientdata(client, NULL);
    return 0;
}

static ssize_t
proc_pca9532_od_write(struct file *file, const char __user * buf,
		   size_t length, loff_t * ppos)
{
    char *buffer;
    int i, err, val, v1, v2;

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
     * Usage: echo "OUTPUT 1-16 0|1|2" >/proc/pca9532_od //2:Blink
     * 
     */
    if (!strncmp(buffer, "OUTPUT", strlen("OUTPUT"))) {
	i = sscanf(buffer + strlen("OUTPUT"), "%d %d\n", &v1, &v2);
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
		pca9532_od_set_led(v1, val);
	    }
	}
    }

    err = length;
  out:
    free_page((unsigned long) buffer);
    *ppos = 0;

    return err;
}

static int proc_pca9532_od_show(struct seq_file *m, void *v)
{
    int i, j=0;
    u8 val1, val2;
    char LED_STATUS[4][8];

    sprintf(LED_STATUS[LED_ON], "ON");
    sprintf(LED_STATUS[LED_OFF], "OFF");
    sprintf(LED_STATUS[LED_BLINK1], "BLINK");
    sprintf(LED_STATUS[LED_BLINK2], "-");

    if (pca9532_od_client) {
	for (j = 0; j < 4; j++) {
	    if (!pca9532_od_rw(pca9532_od_client, PCA9532_REG_LS0 + j, &val1, 0)) {
		for (i = 0; i < 4; i++) {
		    val2 = (val1 >> (i * 2)) & 0x3;
		    seq_printf(m, "OUTPUT#%d: %s\n", j * 4 + (i + 1),
			       LED_STATUS[val2]);
		}
	    }
	}
    }
    return 0;
}

static int proc_pca9532_od_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_pca9532_od_show, NULL);
}

static struct file_operations proc_pca9532_od_operations = {
    .open = proc_pca9532_od_open,
    .read = seq_read,
    .write = proc_pca9532_od_write,
    .llseek = seq_lseek,
    .release = single_release,
};

int pca9532_od_init_procfs(void)
{
    struct proc_dir_entry *pde;

    pde = proc_create("pca9532_od", 0, NULL, &proc_pca9532_od_operations);
    if (pde == NULL) {
         return -ENOMEM;
    }

    return 0;
}

void pca9532_od_exit_procfs(void)
{
    remove_proc_entry("pca9532_od", NULL);
}

static struct i2c_driver pca9532_od_driver = {
    .driver = {
	       .name = "pca9532_od",
	       },
    .probe = pca9532_od_probe,
    .remove = pca9532_od_remove,
    .id_table = pca9532_od_id,

    .class = I2C_CLASS_HWMON,
    .detect = pca9532_od_detect,
    .address_list = normal_i2c,
};

static int __init pca9532_od_init(void)
{
    if (pca9532_od_init_procfs()) {
        printk(KERN_ERR "pca9532_od: cannot create /proc/pca9532_od.\n");
	return -ENOENT;
    }

    printk("pca9532_od_init\n");
    return (i2c_add_driver(&pca9532_od_driver));
}

static void __exit pca9532_od_exit(void)
{
    pca9532_od_exit_procfs();
    i2c_del_driver(&pca9532_od_driver);
}

module_init(pca9532_od_init);
module_exit(pca9532_od_exit);

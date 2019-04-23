/*
 *  Copyright (C) 2006 Thecus Technology Corp. 
 *
 *      Maintainer: citizen <citizen_lee@thecus.com>
 *
 *      porting from thecus N2100 by citizen Lee (citizen_lee@thecus.com)
 *
 *      Written by Y.T. Lee (yt_lee@thecus.com)
 *
 *      add support F75387S by citizen Lee (citizen_lee@thecus.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for Fintek F75387SG1 chip on Thecus 1U4800R/S FAN1/FAN2
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/rtc.h>		/* get the user-level API */
#include <linux/init.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#define F75375_CHIP_ID	0x0306
#define F75387_CHIP_ID	0x0410
static u32 chip_id = F75375_CHIP_ID;


#define DEBUG 1
#define I2C_RETRY 2

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (debug>=x) printk("%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

MODULE_AUTHOR("Y.T. Lee <yt_lee@thecus.com>");
MODULE_DESCRIPTION("Fintek F75387SG1 Driver");
MODULE_LICENSE("GPL");
static int debug;;
module_param(debug, int, S_IRUGO | S_IWUSR);

struct f75387sg1_data {
    struct i2c_client *client;
    struct mutex update_lock;
    u8 ctrl;
};


static struct i2c_client *save_client = NULL;

#define F75387SG_I2C_ID 0x2d
/* Addresses to scan */
static const unsigned short f75387sg1_addr[] = {
    F75387SG_I2C_ID, I2C_CLIENT_END
};

static int pulse = 1;

/* return 0 for no error */
static int f75387sg1_rw(u8 reg_num, u8 * val, int wr)
{
    int ret = 0;
    int i, try_time = I2C_RETRY;

    if (NULL == save_client) {
	printk(KERN_INFO "f75387sg1: i2c_client is NULL\n");
	return 1;
    }

    for (i = 0; i < try_time; i++) {
	if (wr) {		//Write
	    ret = i2c_smbus_write_byte_data(save_client, reg_num, *val);
	} else {		//Read
	    *val = 0;
	    ret = i2c_smbus_read_byte_data(save_client, reg_num);
	    if (ret > 0) {
		*val = ret;
	    }
	}
	if (ret >= 0)
	    break;
	udelay(50);
    }
    if (wr) {			//Write
	if (ret < 0) {
	    printk(KERN_INFO
		   "F75387SG1: cant write F75387SG1 Reg#0x%02X value=%d error (%d)\n",
		   reg_num, *val, ret);
	    return 1;
	}
    } else {			//Read
	if (ret < 0) {
	    printk(KERN_INFO
		   "F75387SG1: cant read F75387SG1 Reg#0x%02X error (%d)\n",
		   reg_num, ret);
	    return 1;
	}
    }
    return 0;
}
EXPORT_SYMBOL(f75387sg1_rw);

static int f75387sg1_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    struct f75387sg1_data *data;
    int ret = 0;
    u8 val1, val2;

    data = kzalloc(sizeof(struct f75387sg1_data), GFP_KERNEL);
    if (!data) {
	ret = -ENOMEM;
	goto exit_free;
    }

    dev_info(&client->dev, "setting platform data\n");
    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    save_client = client;

    printk("Attaching F75387SG1\n");

    val1 = 0;
    ret = f75387sg1_rw(0x5D, &val1, 0);
    if (ret > 0) {
	printk(KERN_INFO "f75387sg1_attach: cant read ctrl\n");
    }
    data->ctrl = val1;
    _DBG(1, "F75387SG1 Vendor ID=%02x\n", val1);

    val1 = 0;
    val2 = 0;
    f75387sg1_rw(0x5A, &val1, 0);
    f75387sg1_rw(0x5B, &val2, 0);
    chip_id = (val1 << 8) | val2;
    printk(KERN_INFO "f753xx: Chip id: %X\n", chip_id);

    if ((chip_id == F75375_CHIP_ID) || (chip_id == F75387_CHIP_ID)) {
      //f75387sg1_rw(0x1, &val1, 0);
      val1 = 0x4F;		// VT1,VT2 is connected to BJT
      f75387sg1_rw(0x1, &val1, 1);
      f75387sg1_rw(0x1, &val1, 0);
      printk(KERN_INFO "reg 0x1: %X\n", val1);

      /* Set to manual mode. */
      val1 = 0x55;
      f75387sg1_rw(0x60, &val1, 1);

      return 0;
    }else{
      printk("f753xx chip id != %X or %X\n",F75375_CHIP_ID,F75387_CHIP_ID);
      kfree(data);
      save_client = NULL;
      return -1;
    }
  exit_free:
    printk("f75387sg1_attach: fail\n");
    kfree(data);
    return ret;
}

static int f75387sg1_remove(struct i2c_client *client)
{
    struct f75387sg1_data *data = i2c_get_clientdata(client);
    kfree(data);
    save_client = NULL;
//    i2c_set_clientdata(client, NULL);
    return 0;
}

static ssize_t proc_f75387sg1_write(struct file *file,
				  const char __user * buf, size_t length,
				  loff_t * ppos)
{
    char *buffer;
    int i, err, v1, v2, v0;
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

    if (!strncmp(buffer, "fan1 speed", strlen("fan1 speed"))) {
	i = sscanf(buffer + strlen("fan1 speed"), "%d", &v2);
	if (i == 1) {
	    if (chip_id == F75375_CHIP_ID) {
		f75387sg1_rw(0x60, &val1, 0);
		// speed mode
		val1 &= ~(0x11 << 4);
		f75387sg1_rw(0x60, &val1, 1);
	    } else if (chip_id == F75387_CHIP_ID) {
		f75387sg1_rw(0x60, &val1, 0);
		// manu mode, pwm, rpm 
		val1 &= ~(0x1 << 2);
		val1 &= ~(0x1 << 1);
		val1 |= (0x1 << 0);
		f75387sg1_rw(0x60, &val1, 1);
	    }
	    v2 = 1500000 / v2;
	    val1 = 0x74;
	    val2 = v2 >> 8;
	    _DBG(1, "Write reg 0x%X=0x%X\n", val1, val2);
	    f75387sg1_rw(val1, &val2, 1);
	    val1 = 0x75;
	    val2 = v2 & 0xff;
	    _DBG(1, "Write reg 0x%X=0x%X\n", val1, val2);
	    f75387sg1_rw(val1, &val2, 1);
	}
    } else if ((!strncmp(buffer, "fan1 pwm", strlen("fan1 pwm")))
	       || (!strncmp(buffer, "fan1 auto", strlen("fan1 auto")))) {
	if (chip_id == F75375_CHIP_ID) {
	    f75387sg1_rw(0x1, &val1, 0);
	    val1 &= ~(0x1 << 4);	// pwm
	    f75387sg1_rw(0x1, &val1, 1);

	    f75387sg1_rw(0x60, &val1, 0);
	    _DBG(1, "1 VAL1=%X\n", val1);
	    val1 &= ~(0x11 << 4);
	    val1 |= (0x1F);
	    _DBG(1, "1 VAL1=%X\n", val1);
	    f75387sg1_rw(0x60, &val1, 1);
	} else if (chip_id == F75387_CHIP_ID) {
	    f75387sg1_rw(0x60, &val1, 0);
	    // auto mode, pwm, duty
	    val1 |= (0x1 << 2);
	    val1 &= ~(0x1 << 1);
	    val1 &= ~(0x1 << 0);
	    f75387sg1_rw(0x60, &val1, 1);

	    val1 = 0xFF;	// set PWM DUTY1 100%
	    f75387sg1_rw(0xA4, &val1, 1);
	    val1 = 0xFF * 50 / 100;	// set PWM DUTY2 50%
	    f75387sg1_rw(0xA5, &val1, 1);
	    val1 = 0xFF * 30 / 100;	// set PWM DUTY3 30%
	    f75387sg1_rw(0xA6, &val1, 1);
	    val1 = 0xFF * 22 / 100;	// set PWM DUTY4 22%
	    f75387sg1_rw(0xA7, &val1, 1);
	    val1 = 0xFF * 0;	// set PWM DUTY5 0%
	    f75387sg1_rw(0xA8, &val1, 1);
	}
    } else if (!strncmp(buffer, "fan1 pulse", strlen("fan1 pulse"))) {
	i = sscanf(buffer + strlen("fan1 pulse"), "%d\n", &v2);
	pulse = v2;
    } else if (!strncmp(buffer, "fan1 duty", strlen("fan1 duty"))) {
	if (chip_id == F75375_CHIP_ID) {
	    i = sscanf(buffer + strlen("fan1 duty"), "%x", &v2);
	    if (i == 1) {
		val2 = v2;
		val1 = 0x76;
		_DBG(1, "Write reg 0x%X=0x%02X\n", val1, val2);
		f75387sg1_rw(val1, &val2, 1);
	    }
	}
    } else if (!strncmp(buffer, "reset", strlen("reset"))) {
	val2 = 0x80;
	_DBG(1, "Write reg 0x%X=0x%02X\n", 0, val2);
	f75387sg1_rw(0, &val2, 1);
	val2 = 0x01;
	_DBG(1, "Write reg 0x%X=0x%02X\n", 0, val2);
	f75387sg1_rw(0, &val2, 1);

	if (chip_id == F75375_CHIP_ID) {
	    //Set reg 0xF0 bit2 to enabled
	    val1 = 0xF0;
	    f75387sg1_rw(val1, &val2, 0);
	    val2 = 0x2;
	    _DBG(1, "Set F75387SG1 register 0x%02X value to 0x%02X  \n", val1,
		 val2);
	    f75387sg1_rw(val1, &val2, 1);
	}
	f75387sg1_rw(0x1, &val1, 0);
	val1 &= ~(0x0C);	// VT1,VT2 is connected to a thermistor
	f75387sg1_rw(0x1, &val1, 1);
    } else if (!strncmp(buffer, "fix", strlen("fix"))) {
	if (chip_id == F75375_CHIP_ID) {
	    val1 = 0x11;
	    f75387sg1_rw(0x6D, &val1, 1);
	}
    } else if (!strncmp(buffer, "BT", strlen("BT"))) {
	i = sscanf(buffer + strlen("BT"), "%d %d\n", &v1, &v2);
	if (i == 2)		//two input
	{
	    val1 = 0xa0 + v1;
	    val2 = v2;
	    _DBG(1, "Write reg 0x%X=0x%X\n", val1, val2);
	    f75387sg1_rw(val1, &val2, 1);
	}
    } else if (!strncmp(buffer, "REG", strlen("REG"))) {
	i = sscanf(buffer + strlen("REG"), "%d %x %x\n", &v0, &v1, &v2);	//R/W: 0/1 : v0
	//Reg addr : v1
	//Reg Value : v2 

	if (i == 3)		//three input
	{
	    val1 = v1;
	    val2 = v2;
	    if (v0 == 1) {
		_DBG(1, "Write reg 0x%02X=0x%02X(%d)\n", val1, val2, val2);
		f75387sg1_rw(val1, &val2, 1);
	    } else {
		val2 = 0;
		f75387sg1_rw(val1, &val2, 0);
		_DBG(1, "Read reg 0x%02X=0x%02X(%d)\n", val1, val2, val2);
	    }
	}
    }

    err = length;
  out:
    free_page((unsigned long) buffer);
    *ppos = 0;

    return err;
}


static int proc_f75387sg1_show(struct seq_file *m, void *v)
{
    int i;
    u8 val1, val2;

    if (NULL == save_client) {
	seq_printf(m, "F75387SG1 not found\n");
	return 0;
    }

    if (!f75387sg1_rw(0x04, &val1, 0)) {
	seq_printf(m, "Address: %02X\n", val1);
    }

    if (!f75387sg1_rw(0x5c, &val1, 0)) {
	seq_printf(m, "Version: %02X\n", val1);
    }

    if (chip_id == F75375_CHIP_ID)
	seq_printf(m, "Chip: F75375\n");
    else if (chip_id == F75387_CHIP_ID)
	seq_printf(m, "Chip: F75387\n");

    if (!f75387sg1_rw(0x14, &val1, 0)) {
	seq_printf(m, "Temp 1: %d\n", val1);
	if (chip_id == F75387_CHIP_ID)
	    f75387sg1_rw(0x1A, &val1, 0);
    }
    if (!f75387sg1_rw(0x15, &val1, 0)) {
	seq_printf(m, "Temp 2: %d\n", val1);
	if (chip_id == F75387_CHIP_ID)
	    f75387sg1_rw(0x1B, &val1, 0);
    }
    if (chip_id == F75387_CHIP_ID) {
	if (!f75387sg1_rw(0x1C, &val1, 0)) {
	    f75387sg1_rw(0x1D, &val2, 0);
	    seq_printf(m, "Temp 3: %d\n", val1);
	}
    }

    if (!f75387sg1_rw(0x74, &val1, 0)) {
	seq_printf(m, "FAN 1 Expected counter(MSB): %02X\n", val1);
    }
    if (!f75387sg1_rw(0x75, &val1, 0)) {
	seq_printf(m, "FAN 1 Expected counter(LSB): %02X\n", val1);
    }

    if (!f75387sg1_rw(0x84, &val1, 0)) {
	seq_printf(m, "FAN 2 Expected counter(MSB): %02X\n", val1);
    }
    if (!f75387sg1_rw(0x85, &val1, 0)) {
	seq_printf(m, "FAN 2 Expected counter(LSB): %02X\n", val1);
    }

    if (chip_id == F75375_CHIP_ID) {
	if (!f75387sg1_rw(0x69, &val1, 0)) {
	    seq_printf(m, "PWM 1 Raise duty: %02X\n", val1);
	}
	if (!f75387sg1_rw(0x6b, &val1, 0)) {
	    seq_printf(m, "PWM 1 Drop duty: %02X\n", val1);
	}
	if (!f75387sg1_rw(0x76, &val1, 0)) {
	    seq_printf(m, "PWM 1 duty: %02X\n", val1);
	}
	if (!f75387sg1_rw(0x60, &val1, 0)) {
	    seq_printf(m, "Reset timer control: %02X\n", val1);
	}
	if (!f75387sg1_rw(0x1, &val1, 0)) {
	    if (val1 & 0x10)
		seq_printf(m, "FAN 1 in Linear mode(%02X)\n", val1);
	    else
		seq_printf(m, "FAN 1 in PWM mode(%02X)\n", val1);
	}
    } else if (chip_id == F75387_CHIP_ID) {
	//seq_printf(m, "PWM 1 Min duty: 0x%02X\n", val1 & 0x0F);
	if (!f75387sg1_rw(0x76, &val1, 0)) {
	    seq_printf(m, "PWM 1 duty: 0x%02X\n", val1);
	}
	if (!f75387sg1_rw(0x86, &val1, 0)) {
	    seq_printf(m, "PWM 2 duty: 0x%02X\n", val1);
	}

	if (!f75387sg1_rw(0x60, &val1, 0)) {
	    seq_printf(m, "FAN1 mode Register: 0x%02X ", val1);
	    if (val1 & 0x01)
		seq_printf(m, " MANU");
	    else
		seq_printf(m, " AUTO");
	    if (val1 & 0x02)
		seq_printf(m, " DAC");
	    else
		seq_printf(m, " PWM");
	    if (val1 & 0x04)
		seq_printf(m, " DUTY");
	    else
		seq_printf(m, " RPM");
	    if (val1 & 0x08)
		seq_printf(m, " OPEN\n");
	    else
		seq_printf(m, " PUSH\n");

	    seq_printf(m, "FAN2 mode Register: 0x%02X ", val1);
	    if (val1 & 0x10)
		seq_printf(m, " MANU");
	    else
		seq_printf(m, " AUTO");
	    if (val1 & 0x20)
		seq_printf(m, " DAC");
	    else
		seq_printf(m, " PWM");
	    if (val1 & 0x40)
		seq_printf(m, " DUTY");
	    else
		seq_printf(m, " RPM");
	    if (val1 & 0x80)
		seq_printf(m, " OPEN\n");
	    else
		seq_printf(m, " PUSH\n");
	}
    }
    if ((!f75387sg1_rw(0x16, &val1, 0))
	&& (!f75387sg1_rw(0x17, &val2, 0))) {
	i = val1;
	i = ((i << 8) & 0xff00) | val2;
	i *= pulse;
	if (i == 0)
	    i = 1;		// avoid divide by zero
	if (((chip_id == F75375_CHIP_ID) && (i == 0xFFFF))
	    || ((chip_id == F75387_CHIP_ID)
		&& (i == 0x0FFF || i == 0x0FFE)))
	    seq_printf(m, "FAN 1 RPM: %d (0x%04X,%d,0x%02X,0x%02X)\n", 0,
		       i, i, val1, val2);
	else
	    seq_printf(m, "FAN 1 RPM: %d (0x%04X,%d,0x%02X,0x%02X)\n",
		       1500000 / i, i, i, val1, val2);
    }
    if ((!f75387sg1_rw(0x18, &val1, 0))
	&& (!f75387sg1_rw(0x19, &val2, 0))) {
	i = val1;
	i = ((i << 8) & 0xff00) | val2;
	i *= pulse;
	if (i == 0)
	    i = 1;		// avoid divide by zero
	if (((chip_id == F75375_CHIP_ID) && (i == 0xFFFF))
	    || ((chip_id == F75387_CHIP_ID)
		&& (i == 0x0FFF || i == 0x0FFE)))
	    seq_printf(m, "FAN 2 RPM: %d (0x%04X,%d,0x%02X,0x%02X)\n", 0,
		       i, i, val1, val2);
	else
	    seq_printf(m, "FAN 2 RPM: %d (0x%04X,%d,0x%02X,0x%02X)\n",
		       1500000 / i, i, i, val1, val2);
    }
    if ((!f75387sg1_rw(0x70, &val1, 0))
	&& (!f75387sg1_rw(0x71, &val2, 0))) {
	i = val1;
	i = ((i << 8) & 0xff00) | val2;
	if (i == 0)
	    i = 1;		// avoid divide by zero
	if (((chip_id == F75375_CHIP_ID) && (i == 0xFFFF))
	    || ((chip_id == F75387_CHIP_ID) && (i == 0x0FFF)))
	    seq_printf(m, "FAN 1 top RPM: %d (0x%02X,%d)\n", 0, i, i);
	else
	    seq_printf(m, "FAN 1 top RPM: %d (0x%02X,%d)\n", 1500000 / i,
		       i, i);
    }
    if ((!f75387sg1_rw(0x80, &val1, 0))
	&& (!f75387sg1_rw(0x81, &val2, 0))) {
	i = val1;
	i = ((i << 8) & 0xff00) | val2;
	if (i == 0)
	    i = 1;		// avoid divide by zero
	if (((chip_id == F75375_CHIP_ID) && (i == 0xFFFF))
	    || ((chip_id == F75387_CHIP_ID) && (i == 0x0FFF)))
	    seq_printf(m, "FAN 2 top RPM: %d (0x%02X,%d)\n", 0, i, i);
	else
	    seq_printf(m, "FAN 2 top RPM: %d (0x%02X,%d)\n", 1500000 / i,
		       i, i);
    }
    for (i = 0; i < 0xe; i++) {
	val2 = 0xa0 + i;
	if (!f75387sg1_rw(val2, &val1, 0))
	    seq_printf(m, "Reg 0x%2X: 0x%02X(%3d)", val2, val1, val1);
	seq_printf(m, "\n");
    }

    return 0;
}

static int proc_f75387sg1_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_f75387sg1_show, NULL);
}

static struct file_operations proc_f75387sg1_operations = {
    .open = proc_f75387sg1_open,
    .read = seq_read,
    .write = proc_f75387sg1_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct i2c_device_id f75387sg1_id[] = {
    {"F75387SG1", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, f75387sg1_id);

/* Return 0 if detection is successful, -ENODEV otherwise */
static int f75387sg1_detect(struct i2c_client *client,
                          struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;
    unsigned short address = client->addr;
    const char *chip_name = "F75387SG1";

    strlcpy(info->type, chip_name, I2C_NAME_SIZE);
    dev_info(&adapter->dev, "Found %s at 0x%02hx\n", chip_name, address);

    return 0;
}

static struct i2c_driver f75387sg1_driver = {
    .driver = {
	       .name = "F75387SG1",
	       },
    .probe = f75387sg1_probe,
    .remove = f75387sg1_remove,
    .id_table = f75387sg1_id,

    .class = I2C_CLASS_HWMON,
    .detect = f75387sg1_detect,
    .address_list = f75387sg1_addr,
};


static __init int f75387sg1_init(void)
{
    struct proc_dir_entry *pde;

    pde = proc_create("f75387sg1", 0, NULL, &proc_f75387sg1_operations);
    if (pde == NULL) {
	printk(KERN_ERR "F75387SG1: cannot create /proc/f75387sg1.\n");
        return -ENOMEM;
    }

    return i2c_add_driver(&f75387sg1_driver);
}

static __exit void f75387sg1_exit(void)
{
    remove_proc_entry("f75387sg1", NULL);
    i2c_del_driver(&f75387sg1_driver);
}

module_init(f75387sg1_init);
module_exit(f75387sg1_exit);

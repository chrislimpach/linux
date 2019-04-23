/*
 *  Copyright (C) 2006 Thecus Technology Corp. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Driver for Thecus virtual hwm
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/rtc.h>		/* get the user-level API */
#include <linux/init.h>
#include <linux/types.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (debug>=x) printk("%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

MODULE_AUTHOR("Joey Wang<joey_wang@thecus.com>");
MODULE_DESCRIPTION("Thecus virtual hwm Driver");
MODULE_LICENSE("GPL");
static int debug;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int fan[7]={0,0,0,0,0,0,0};
static int temp[5]={0,0,0,0,0};

static ssize_t proc_thecus_hwm_write(struct file *file,
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

    if (!strncmp(buffer, "REG", strlen("REG"))) {
	i = sscanf(buffer + strlen("REG"), "%d %x %x\n", &v0, &v1, &v2);	//R/W: 0/1 : v0
	//Reg addr : v1
	//Reg Value : v2 
	if (i == 3)		//three input
	{
	    val1 = v1;
	    val2 = v2;
	    if (v0 == 1) {
		printk("Write reg 0x%02X=0x%02X(%d)\n", val1, val2, val2);
	    } else {
		val2 = 0;
		printk("Read reg 0x%02X=0x%02X(%d)\n", val1, val2, val2);
	    }
	}
    }else if (!strncmp(buffer, "FAN", strlen("FAN"))) {
	i = sscanf(buffer + strlen("FAN"), "%d %d\n", &v0, &v1);
	if (i == 2 ){
          if ((v0 >= 0) && (v0 <= 6))
            fan[v0]=v1;
        }
    }else if (!strncmp(buffer, "TEMP", strlen("TEMP"))) {
	i = sscanf(buffer + strlen("TEMP"), "%d %d\n", &v0, &v1);
	if (i == 2 ){
          if ((v0 >= 0) && (v0 <= 4))
            temp[v0]=v1;
        }
    }

    err = length;
  out:
    free_page((unsigned long) buffer);
    *ppos = 0;

    return err;
}


static int proc_thecus_hwm_show(struct seq_file *m, void *v)
{
    seq_printf(m,"Display Thecus virtual hwm Info Ver.\n");

    seq_printf(m,"CPU_FAN RPM: %d\n", fan[0]);
    seq_printf(m,"SYS_FAN1 RPM: %d\n", fan[1]);
    seq_printf(m,"SYS_FAN2 RPM: %d\n", fan[2]);
    seq_printf(m,"HDD_FAN1 RPM: %d\n", fan[3]);
    seq_printf(m,"HDD_FAN2 RPM: %d\n", fan[4]);
    seq_printf(m,"HDD_FAN3 RPM: %d\n", fan[5]);
    seq_printf(m,"HDD_FAN4 RPM: %d\n", fan[6]);

    seq_printf(m,"CPU_TEMP: %d\n", temp[0]);
    seq_printf(m,"SAS_TEMP: %d\n", temp[1]);
    seq_printf(m,"SYS_TEMP: %d\n", temp[2]);
    seq_printf(m,"HDD_TEMP1: %d\n", temp[3]);
    seq_printf(m,"HDD_TEMP2: %d\n", temp[4]);

    return 0;
}

static int proc_thecus_hwm_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_thecus_hwm_show, NULL);
}

static struct file_operations proc_thecus_hwm_operations = {
    .open = proc_thecus_hwm_open,
    .read = seq_read,
    .write = proc_thecus_hwm_write,
    .llseek = seq_lseek,
    .release = single_release,
};

static __init int thecus_hwm_init(void)
{
    struct proc_dir_entry *pde;

    pde = proc_create("hwm", 0, NULL, &proc_thecus_hwm_operations);
    if (pde == NULL) {
	printk(KERN_ERR "THECUS HWM: cannot create /proc/hwm.\n");
        return -ENOMEM;
    }

    return 0;
}

static __exit void thecus_hwm_exit(void)
{
    remove_proc_entry("hwm", NULL);
}

module_init(thecus_hwm_init);
module_exit(thecus_hwm_exit);

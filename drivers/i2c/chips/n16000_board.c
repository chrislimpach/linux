/*
 *  Copyright (C) 2009 Thecus Technology Corp. 
 *
 *      Maintainer: citizen <citizen_lee@thecus.com>
 *
 *      Driver for Thecus N16000
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
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
#include <linux/notifier.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/vt_kern.h>

#include "thecus_board.h"

u32 n16000_disk_access(int index, int act);
u32 n16000_disk_index(int index, struct scsi_device *sdp);

static const struct thecus_function n16000_func = {
        .disk_access = n16000_disk_access,
        .disk_index  = n16000_disk_index,
};

static int board_idx = 0;
static struct thecus_board board_info [] = {
	{ 0, "N16000", 16, 17, 2, "600", "BOARD_N16000", &n16000_func},
	{ 1, "N12000", 12, 17, 2, "601", "BOARD_N16000", &n16000_func},
	{ }
};

extern int PCH_GPIO_init(void);
extern void PCH_GPIO_exit(void);
extern int PCH_gpio_read_bit(u32 bit_n);
extern int PCH_gpio_write_bit(u32 bit_n, int val);
extern void pca9532_set_led(int led_num, int led_state);
extern u32 thecus_board_register(struct thecus_board *board);
extern u32 thecus_board_unregister(struct thecus_board *board);

//#define DEBUG 1

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (DEBUG>=x) printk("%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

MODULE_AUTHOR("citizen Lee <citizen_lee@thecus.com>");
MODULE_DESCRIPTION
    ("Thecus N16000 MB Driver and board depend io operation");
MODULE_LICENSE("GPL");
static int debug;;
module_param(debug, int, S_IRUGO | S_IWUSR);

static u32 keep_BUZZER = BUZZER_OFF;
static u32 led_sys_busy = LED_PCH_OFF;
static u32 led_sys_err = LED_PCH_OFF;
static u32 led_blink_value = 0;

u32 n16000_disk_access(int index,int act){
    // disk access led by hw
    return 1;
}

u32 n16000_disk_index(int index, struct scsi_device *sdp){
    u32 tindex=index;
    if(0==strncmp(sdp->host->hostt->name,"ahci",strlen("ahci"))){
        tindex = sdp->host->host_no;
        switch (tindex) {
            case 0: // DOMA
            case 1: // DOMB
                tindex = tindex + 702 + sdp->channel + sdp->id + sdp->lun;
                break;
            case 2:
            case 3:
                tindex = tindex + 16 + sdp->channel + sdp->id + sdp->lun;
                break;
            case 4: // eSATA1
            case 5: // eSATA2
                tindex = tindex + 12 + sdp->channel + sdp->id + sdp->lun;
                break;
        }
    }else if(0==strncmp(sdp->host->hostt->name,"Fusion MPT SAS Host",strlen("Fusion MPT SAS Host"))){
        // 0 ~ 15
        tindex = sdp->tray_id - 1;
    }else if(0==strncmp(sdp->host->hostt->name,"uas",strlen("uas"))){
        sdp->disk_if = 2;      // Let UAS = USB
    }
    return tindex;
}

static ssize_t proc_thecus_io_write(struct file *file,
				    const char __user * buf, size_t length,
				    loff_t * ppos)
{
    char *buffer, buf1[20];
    int i, err, v1, v2;
    u8 val;

    if (!buf || length > PAGE_SIZE)
	return -EINVAL;

    err = -ENOMEM;
    buffer = (char *) __get_free_page(GFP_KERNEL);
    if (!buffer)
	goto out2;

    err = -EFAULT;
    if (copy_from_user(buffer, buf, length))
	goto out;

    err = -EINVAL;
    if (length < PAGE_SIZE) {
	buffer[length] = '\0';
#define LF	0xA
	if (length > 0 && buffer[length - 1] == LF)
	    buffer[length - 1] = '\0';
    } else if (buffer[PAGE_SIZE - 1])
	goto out;

    memset(buf1, 0, sizeof(buf1));

    /*
     $ Usage: echo "S_LED 1-16 0|1|2" >/proc/thecus_io //2:Blink * LED SATA 1-16 ERROR led
     * Usage: echo "Fail 0|1" >/proc/thecus_io                  * LED System Fail
     * Usage: echo "Busy 0|1" >/proc/thecus_io                  * LED System Busy
     * Usage: echo "Buzzer 0|1" >/proc/thecus_io                * Buzzer
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
    } else if (!strncmp(buffer, "Busy", strlen("Busy"))) {
	i = sscanf(buffer + strlen("Busy"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "Busy %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_PCH_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_PCH_ON;
	    else
		val = LED_PCH_BLINK;

	    led_sys_busy = val;
	    PCH_gpio_write_bit(35, val);
	}
    } else if (!strncmp(buffer, "Fail", strlen("Fail"))) {
	i = sscanf(buffer + strlen("Fail"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "Fail %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_PCH_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_PCH_ON;
	    else
		val = LED_PCH_BLINK;

	    led_sys_err = val;
	    PCH_gpio_write_bit(27, val);
	}
    } else if (!strncmp(buffer, "Buzzer", strlen("Buzzer"))) {
	i = sscanf(buffer + strlen("Buzzer"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "Buzzer %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = BUZZER_OFF;
	    else
		val = BUZZER_ON;

	    keep_BUZZER = val;
	    kd_mksound(val * 440, 0);
	}
    } else if (!strncmp(buffer, "HA_EN", strlen("HA_EN"))) {
        i = sscanf(buffer + strlen("HA_EN"), "%d\n", &v1);
        if (i == 1)             //only one input
        {
            _DBG(1, "ENABLE_HA_MD %d\n", v1);
            thecus_ha_enable=v1;
        }
    }

    err = length;
  out:
    free_page((unsigned long) buffer);
  out2:
    *ppos = 0;

    return err;
}


static int proc_thecus_io_show(struct seq_file *m, void *v)
{
    u8 val = 0;
    char LED_PCH_STATUS[3][8];

    sprintf(LED_PCH_STATUS[LED_PCH_ON], "ON");
    sprintf(LED_PCH_STATUS[LED_PCH_OFF], "OFF");
    sprintf(LED_PCH_STATUS[LED_PCH_BLINK], "BLINK");

    seq_printf(m, "MODELNAME: %s\n", board_info[board_idx].board_string);

    seq_printf(m, "FAC_MODE: OFF\n");

    seq_printf(m, "Buzzer: %s\n", keep_BUZZER ? "ON" : "OFF");

    seq_printf(m, "MAX_TRAY: %d\n", board_info[board_idx].max_tray);
    seq_printf(m, "eSATA_TRAY: %d\n", board_info[board_idx].eSATA_tray);
    seq_printf(m, "eSATA_COUNT: %d\n", board_info[board_idx].eSATA_count);
    seq_printf(m, "WOL_FN: %d\n", 1);
    seq_printf(m, "FAN_FN: %d\n", 1);
    seq_printf(m, "BEEP_FN: %d\n", 1);
    seq_printf(m, "eSATA_FN: %d\n", 1);
    seq_printf(m, "MBTYPE: %s\n", board_info[board_idx].mb_type);

    val = PCH_gpio_read_bit(57);  // PSU_FAIL gpio
    if(val>=0) seq_printf(m,"PSU1_FAIL: %s\n", val?"OFF":"ON");

    seq_printf(m, "LED_Busy: %s\n", LED_PCH_STATUS[led_sys_busy]);
    seq_printf(m, "LED_Fail: %s\n", LED_PCH_STATUS[led_sys_err]);

    seq_printf(m, "HA_EN: %d\n", thecus_ha_enable);
    seq_printf(m, "CONSOLE: %s\n", "ttyS1");

//#ifdef DEBUG
    val = PCH_gpio_read_bit(35);
    seq_printf(m, "GPIO35: %s\n", val ? "HIGH" : "LOW");
    val = PCH_gpio_read_bit(27);
    seq_printf(m, "GPIO27: %s\n", val ? "HIGH" : "LOW");
//#endif

    val = PCH_gpio_read_bit(21);
    seq_printf(m, "GPIO21: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(22);
    seq_printf(m, "GPIO22: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(37);
    seq_printf(m, "GPIO37: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(38);
    seq_printf(m, "GPIO38: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(39);
    seq_printf(m, "GPIO39: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(48);
    seq_printf(m, "GPIO48: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(49);
    seq_printf(m, "GPIO49: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(56);
    seq_printf(m, "GPIO56: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(57);	// psu fail
    seq_printf(m, "GPIO57: %s\n", val ? "HIGH" : "LOW");

    val = PCH_gpio_read_bit(36);	// mute button
    seq_printf(m, "GPIO36: %s\n", val ? "HIGH" : "LOW");

    return 0;
}

static int proc_thecus_io_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_thecus_io_show, NULL);
}

static struct file_operations proc_thecus_io_operations = {
    .open = proc_thecus_io_open,
    .read = seq_read,
    .write = proc_thecus_io_write,
    .llseek = seq_lseek,
    .release = single_release,
};

// ----------------------------------------------------------
static DECLARE_WAIT_QUEUE_HEAD(thecus_event_queue);
#define MESSAGE_LENGTH 80
static char Message[MESSAGE_LENGTH];
#define MY_WORK_QUEUE_NAME "btn_sched"	// length must < 10
#define WORK_QUEUE_TIMER_1 250
#define WORK_QUEUE_TIMER_2 50
static u32 dyn_work_queue_timer = WORK_QUEUE_TIMER_1;
static void intrpt_routine(struct work_struct *unused);
static int module_die = 0;	/* set this to 1 for shutdown */
static struct workqueue_struct *my_workqueue;
static struct delayed_work Task;
static DECLARE_DELAYED_WORK(Task, intrpt_routine);

static void intrpt_routine(struct work_struct *unused)
{
    static u32 psu_status = 1;	// default high, psu fail 時會 low
    u8 val = 0;

    if (led_sys_busy == LED_PCH_BLINK) {
	PCH_gpio_write_bit(35, led_blink_value % 2);
    }
    if (led_sys_err == LED_PCH_BLINK) {
	PCH_gpio_write_bit(27, led_blink_value % 2);
    }
    led_blink_value++;

    val = PCH_gpio_read_bit(57);	// PSU_FAIL gpio
    if(psu_status != val) {
        // default high, psu fail 時會 low
        if(val == 1) {
            sprintf(Message, "PSU_FAIL OFF\n");
        } else {
            sprintf(Message, "PSU_FAIL ON\n");
        }
        psu_status = val;
        wake_up_interruptible(&thecus_event_queue);
    }
    if(PCH_gpio_read_bit(36)==0) {	// mute button
	    kd_mksound(0 * 440, 0);
	    keep_BUZZER = 0;
    }

    // If cleanup wants us to die
    if (module_die == 0)
	queue_delayed_work(my_workqueue, &Task, dyn_work_queue_timer);

}


static ssize_t thecus_event_read(struct file *file, char __user * buffer,
				 size_t length, loff_t * ppos)
{
    static int finished = 0;
    int i;
    if (finished) {
	finished = 0;
	return 0;
    }
//      printk(KERN_DEBUG "process %i (%s) going to sleep\n",
//           current->pid, current->comm);
    interruptible_sleep_on(&thecus_event_queue);
//      printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
    for (i = 0; i < length && Message[i]; i++)
	put_user(Message[i], buffer + i);

    finished = 1;
    return i;
}

static struct file_operations proc_thecus_event_operations = {
    .read = thecus_event_read,
};


static __init int thecus_io_init(void)
{
    struct proc_dir_entry *pde;
    u32 board_num = 0, n = 0;
    u8 val;
    int ret;

    ret = PCH_GPIO_init();
    if (ret < 0) {
//        printk(KERN_ERR "PCH_GPIO_init failed\n");
        return ret;
    }

    pde = proc_create("thecus_io", 0, NULL, &proc_thecus_io_operations);
    if (pde == NULL) {
        printk(KERN_ERR "thecus_io: cannot create /proc/thecus_io.\n");
        return -ENOMEM;
    }

    pde = proc_create("thecus_event", 0, NULL, &proc_thecus_event_operations);
    if (pde == NULL) {
        printk(KERN_ERR "thecus_io: cannot create /proc/thecus_event.\n");
        return -ENOMEM;
    }

    my_workqueue = create_singlethread_workqueue(MY_WORK_QUEUE_NAME);
    if (my_workqueue) {
	queue_delayed_work(my_workqueue, &Task, dyn_work_queue_timer);
	init_waitqueue_head(&thecus_event_queue);
    } else {
	printk(KERN_ERR "thecus_io: error in thecus_io_init\n");
    }

    // load driver, initial blink busy led
    led_sys_busy = LED_PCH_BLINK;
    PCH_gpio_write_bit(35, LED_PCH_BLINK);

    val = PCH_gpio_read_bit(21);
    board_num |= val;
    val = PCH_gpio_read_bit(22);
    board_num |= (val<<1);
    val = PCH_gpio_read_bit(37);
    board_num |= (val<<2);
    val = PCH_gpio_read_bit(38);
    board_num |= (val<<3);

    printk(KERN_INFO "thecus_io: board_num: %Xh\n", board_num);
    for (n = 0; board_info[n].board_string; n++)
        if (board_num == board_info[n].gpio_num) {
            board_idx = n;
            break;
        }

    thecus_board_register(&board_info[board_idx]);

    return 0;
}

static __exit void thecus_io_exit(void)
{
    module_die = 1;		// keep intrp_routine from queueing itself 
    remove_proc_entry("thecus_io", NULL);
    remove_proc_entry("thecus_event", NULL);

    cancel_delayed_work(&Task);	// no "new ones" 
    flush_workqueue(my_workqueue);	// wait till all "old ones" finished 
    destroy_workqueue(my_workqueue);

    PCH_GPIO_exit();

    thecus_board_unregister(&board_info[board_idx]);
}

module_init(thecus_io_init);
module_exit(thecus_io_exit);

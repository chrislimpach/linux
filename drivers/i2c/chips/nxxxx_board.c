/*
 *  Copyright (C) 2009 Thecus Technology Corp. 
 *
 *      Maintainer: joey <joey_wang@thecus.com>
 *
 *      Driver for Thecus NXXXX board's I/O
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
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
#include <linux/notifier.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/vt_kern.h>
#include <linux/reboot.h>

#include "thecus_board.h"

u32 hw_disk_access(int index, int act);
u32 n2810_disk_index(int index, struct scsi_device *sdp);

static const struct thecus_function n2810_func = {
        .disk_access = hw_disk_access,
        .disk_index  = n2810_disk_index,
};

static int board_idx = 0;
static struct thecus_board board_info [] = {
/*  { gpio_num, board_string, max_tray, eSATA_tray, eSATA_count, mb_type, name, thecus_function},*/
    { 1200    , "N2810"     , 2       , 17        , 0          , "1200" , "BOARD_N2810", &n2810_func},
    { }
};

#define GPIO_BRASWELL 0

static struct thecus_fn board_fn [] = {
/*  { gpio_num, gpio_type    , copy_btn, mute_btn, reset_btn, psu, battery, up_type, up_port, debug_port},*/
    { 1200    , GPIO_BRASWELL, 0       , 0       , 0        , 0  , 0      , "NONE" , "NONE" , "ttyS0"},
    { }
};

static struct thecus_gpio board_gpio [] = {
/*  { usb_err_led, usb_acs_led, pwr_err_led, pwr_st_led, hdd1_err_led, hdd2_err_led, copy_btn, mbid1, mbid2, mbid3},*/
    { 104, 116, 22, 42, 98, 101, 113, 158, 163, 170},
    {}
};

#define BW_SOC_LED_OFF 9470207
#define BW_SOC_LED_ON  9470209

extern int braswell_GPIO_init(void);
extern int braswell_GPIO_exit(void);

extern u32 thecus_board_register(struct thecus_board *board);
extern u32 thecus_board_unregister(struct thecus_board *board);

extern void braswell_gpio_set_alt(int gpio, int alt);
extern int braswell_gpio_get_alt(int gpio);

#define DEBUG 0

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (DEBUG>=x) printk("%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

MODULE_AUTHOR("Joey Wang <joey_wang@thecus.com>");
MODULE_DESCRIPTION
    ("Thecus NXXXX MB Driver and board depend io operation");
MODULE_LICENSE("GPL");
static int debug;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int ext_board_id=0;
module_param(ext_board_id, int, S_IRUGO | S_IWUSR);

static u32 keep_BUZZER = BUZZER_OFF;
static u32 led_usb_busy = LED_OFF;
static u32 led_usb_err = LED_OFF;
static u32 led_sd_busy = LED_OFF;
static u32 led_sd_err = LED_OFF;
static u32 led_logo1 = LED_OFF;
static u32 led_logo2 = LED_OFF;
static u32 led_sys_power = LED_OFF;
static u32 led_sys_busy = LED_OFF;
static u32 led_sys_err = LED_OFF;
static int sleepon_flag=0;
static int bat_flag=0;

static u32 access_led[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

u32 hw_disk_access(int index,int act){
    /* HW Support */
    return 0;
}

u32 n2810_disk_index(int index, struct scsi_device *sdp){
    u32 tindex=index;
    if(0==strncmp(sdp->host->hostt->name,"ahci",strlen("ahci"))){
        tindex = sdp->host->host_no; // For HDD (1~5)
    }else if(0==strncmp(sdp->host->hostt->name,"uas",strlen("uas"))){
        sdp->disk_if = 2;      // Let UAS = USB
    }
    return tindex;
}

static void io_set_disk_err_led(int led_num, int led_state)
{
    int idx=0;
    if (board_fn[board_idx].gpio_type == GPIO_BRASWELL){
        switch (led_num) {
        case 0: 
	    idx=board_gpio[board_idx].hdd1_err_led;
            break;
        case 1:
	    idx=board_gpio[board_idx].hdd2_err_led;   
            break;
        }
        
        if (led_state == 0)
            braswell_gpio_set_alt(idx,BW_SOC_LED_OFF);
        else 
            braswell_gpio_set_alt(idx,BW_SOC_LED_ON);
    }
    return;
}

static int io_get_input(int led_num)
{
    int ret;
    ret=braswell_gpio_get_alt(led_num) & 0x00000001;
    return ret;
}

static void io_set_output(int led_num, int led_state)
{
    if (board_fn[board_idx].gpio_type == GPIO_BRASWELL){
        if (led_state == 0)
            braswell_gpio_set_alt(led_num,BW_SOC_LED_OFF);
        else 
            braswell_gpio_set_alt(led_num,BW_SOC_LED_ON);
    }
    return;
}


static ssize_t proc_thecus_io_write(struct file *file,
				    const char __user * buf, size_t length,
				    loff_t * ppos)
{
    char *buffer, buf1[20];
    int i, err, v1, v2, v3;
    u8 val=0;

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
     * Usage: echo "S_LED 1-12 0|1|2" >/proc/thecus_io //2:Blink * LED SATA 1-12 ERROR led
     * Usage: echo "U_LED 0|1|2" >/proc/thecus_io //2:Blink * USB BUSY led
     * Usage: echo "UF_LED 0|1|2" >/proc/thecus_io //2:Blink * USB ERROR led
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

	    v1 = v1 - 1;
	    io_set_disk_err_led(v1, val);
	}
    } else if (!strncmp(buffer, "GPIO", strlen("GPIO"))) {
	i = sscanf(buffer + strlen("GPIO"), "%d %d %d\n", &v1, &v2, &v3);
	if (i == 3)		//3 input
	{
	    if (v1 == 0){ //read
		printk("GPIO READ %d %x\n",v2,braswell_gpio_get_alt(v2));
	    }else if (v1 == 1){ //write
		printk("GPIO WRITE %d %x\n",v2,v3);
		braswell_gpio_set_alt(v2,v3);
	    }
	}
    } else if (!strncmp(buffer, "U_LED", strlen("U_LED"))) {
	i = sscanf(buffer + strlen("U_LED"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "U_LED %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_ON;
	    else
		val = LED_BLINK1;

	    led_usb_busy = val;
	    io_set_output(board_gpio[board_idx].usb_acs_led, val);
	}
    } else if (!strncmp(buffer, "UF_LED", strlen("UF_LED"))) {
	i = sscanf(buffer + strlen("UF_LED"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "UF_LED %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_ON;
	    else
		val = LED_BLINK1;

	    led_usb_err = val;
	    io_set_output(board_gpio[board_idx].usb_err_led, val);
	}
    } else if (!strncmp(buffer, "Busy", strlen("Busy"))) {
	i = sscanf(buffer + strlen("Busy"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "Busy %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_ON;
	    else
		val = LED_BLINK1;

	    led_sys_busy = val;
	    //io_set_output(board_gpio[board_idx].pwr_st_led, val);
	}
    } else if (!strncmp(buffer, "Fail", strlen("Fail"))) {
	i = sscanf(buffer + strlen("Fail"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "Fail %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_ON;
	    else
		val = LED_BLINK1;

	    led_sys_err = val;
	    //io_set_output(board_gpio[board_idx].pwr_err_led, val);
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
	    //io_set_output(8, val);
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
    char LED_STATUS[4][8];

    sprintf(LED_STATUS[LED_ON], "ON");
    sprintf(LED_STATUS[LED_OFF], "OFF");
    sprintf(LED_STATUS[LED_BLINK1], "BLINK");
    sprintf(LED_STATUS[LED_BLINK2], "-");

    seq_printf(m, "MODELNAME: %s\n", board_info[board_idx].board_string);

    seq_printf(m, "FAC_MODE: OFF\n");

    seq_printf(m, "Buzzer: %s\n", keep_BUZZER ? "ON" : "OFF");

    seq_printf(m, "MAX_TRAY: %d\n", board_info[board_idx].max_tray);
    seq_printf(m, "eSATA_FN: %d\n", board_info[board_idx].eSATA_count ? 1 : 0);
    seq_printf(m, "eSATA_TRAY: %d\n", board_info[board_idx].eSATA_tray);
    seq_printf(m, "eSATA_COUNT: %d\n", board_info[board_idx].eSATA_count);
    seq_printf(m, "WOL_FN: %d\n", 1);
    seq_printf(m, "FAN_FN: %d\n", 1);
    seq_printf(m, "BEEP_FN: %d\n", 1);
    seq_printf(m, "MBTYPE: %s\n", board_info[board_idx].mb_type);

    seq_printf(m, "U_LED: %s\n", LED_STATUS[led_usb_busy]);
    seq_printf(m, "UF_LED: %s\n", LED_STATUS[led_usb_err]);
    seq_printf(m, "LED_Busy: %s\n", LED_STATUS[led_sys_busy]);
    seq_printf(m, "LED_Fail: %s\n", LED_STATUS[led_sys_err]);
    seq_printf(m, "BAT_FN: %d\n", board_fn[board_idx].battery);
    seq_printf(m, "CONSOLE: %s\n", board_fn[board_idx].debug_port);

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
    u8 val = 0;

    val = io_get_input(board_gpio[board_idx].copy_btn);
    if(val == 0) {
        sprintf(Message, "Copy ON\n");
        wake_up_interruptible(&thecus_event_queue);
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
    sleepon_flag=1;
    interruptible_sleep_on(&thecus_event_queue);
    sleepon_flag=0;
    for (i = 0; i < length && Message[i]; i++)
	put_user(Message[i], buffer + i);

    finished = 1;
    return i;
}

static struct file_operations proc_thecus_event_operations = {
    .read = thecus_event_read,
};


static int sys_notify_reboot(struct notifier_block *nb, unsigned long event, void *p)
{

    switch (event) {
    case SYS_RESTART:
    case SYS_HALT:
    case SYS_POWER_OFF:
        // turn off busy/err led
        break;
    }
    return NOTIFY_DONE;
}

static struct notifier_block sys_notifier_reboot = {
    .notifier_call = sys_notify_reboot,
    .next = NULL,
    .priority = 0
};

static __init int thecus_io_init(void)
{
    struct proc_dir_entry *pde;
    u32 board_num = 0, n = 0;
    u8 val;
    int ret;
    
    if (ext_board_id == 0) {
	printk("Skip NXXXX init!\n");
        return -1;
    }

    if (board_fn[board_idx].gpio_type == GPIO_BRASWELL){
        ret = braswell_GPIO_init();
        if (ret < 0) {
            printk("braswell_GPIO_init failed\n");
            return ret;
        }

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

    board_num = ext_board_id;

    printk(KERN_INFO "thecus_io: board_num: %Xh\n", board_num);
    for (n = 0; board_info[n].board_string; n++)
        if (board_num == board_info[n].gpio_num) {
            board_idx = n;
            break;
        }

    thecus_board_register(&board_info[board_idx]);

    /* turn off all led */

    register_reboot_notifier(&sys_notifier_reboot);

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

    if (board_fn[board_idx].gpio_type == GPIO_BRASWELL){
        braswell_GPIO_exit();
    }

    unregister_reboot_notifier(&sys_notifier_reboot);

    thecus_board_unregister(&board_info[board_idx]);
}

module_init(thecus_io_init);
module_exit(thecus_io_exit);

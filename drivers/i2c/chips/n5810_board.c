/*
 *  Copyright (C) 2009 Thecus Technology Corp. 
 *
 *      Maintainer: joey <joey_wang@thecus.com>
 *
 *      Driver for Thecus N5810 board's I/O
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
#include <linux/reboot.h>

#include <linux/thecus_event.h>

#include "thecus_board.h"

static u32 hw_disk_access(int index, int act);
u32 n5810_disk_index(int index, struct scsi_device *sdp);
u32 n4810_disk_access(int index, int act);
u32 n4810_disk_index(int index, struct scsi_device *sdp);

static const struct thecus_function n5810_func = {
        .disk_access = hw_disk_access,
        .disk_index  = n5810_disk_index,
};

static const struct thecus_function n4810_func = {
        .disk_access = n4810_disk_access,
        .disk_index  = n4810_disk_index,
};

static int board_idx = 0;
static int battery_board_index[]={0,2,7};
static struct thecus_board board_info [] = {
	{ 0, "N5810PRO"    , 5, 17, 0, "1000", "BOARD_N5810", &n5810_func},
	{ 1, "N5810"       , 5, 17, 0, "1001", "BOARD_N5810", &n5810_func},
	{ 2, "N5810PROA"   , 5, 17, 0, "1002", "BOARD_N5810", &n5810_func},
	{ 3, "N4810U-R"    , 4, 17, 0, "1003", "BOARD_N4810", &n4810_func},
	{ 4, "N4810U-S"    , 4, 17, 0, "1004", "BOARD_N4810", &n4810_func},
	{ 5, "N5810D"      , 5, 17, 0, "1005", "BOARD_N5810", &n5810_func},
	{ 6, "N5810T"      , 5, 17, 0, "1006", "BOARD_N5810", &n5810_func},
	{ 7, "N5810PROT"   , 5, 17, 0, "1007", "BOARD_N5810", &n5810_func},
	{ 8, "N5810DT"     , 5, 17, 0, "1008", "BOARD_N5810", &n5810_func},
	{ }
};

extern int SOC_GPIO_init(void);
extern void SOC_GPIO_exit(void);
extern int SOC_gpio_read_bit(u32 bit_n);
extern int SOC_gpio_write_bit(u32 bit_n, int val);
extern void pca9532_set_led(int led_num, int led_state);
extern void pca9532_id_set_led(int led_num, int led_state);
extern void pca9532_od_set_led(int led_num, int led_state);
extern int pca9532_id_get_led(int led_num);
extern int pca9532_id_get_input(int led_num);
extern int pca9532_od_get_led(int led_num);
extern int pca9532_od_get_input(int led_num);
extern u32 thecus_board_register(struct thecus_board *board);
extern u32 thecus_board_unregister(struct thecus_board *board);

//#define DEBUG 1

#ifdef DEBUG
# define _DBG(x, fmt, args...) do{ if (DEBUG>=x) printk("%s: " fmt "\n", __FUNCTION__, ##args); } while(0);
#else
# define _DBG(x, fmt, args...) do { } while(0);
#endif

MODULE_AUTHOR("Joey Wang <joey_wang@thecus.com>");
MODULE_DESCRIPTION
    ("Thecus N5810 MB Driver and board depend io operation");
MODULE_LICENSE("GPL");
static int debug;;
module_param(debug, int, S_IRUGO | S_IWUSR);

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

u32 n5810_disk_index(int index, struct scsi_device *sdp){
    u32 tindex=index;
    if(0==strncmp(sdp->host->hostt->name,"usb-storage",strlen("usb-storage"))){
      if(0==strncmp(sdp->host->hostt->info(sdp->host),"usb-storage 1-1:1.0",strlen("usb-storage 1-1:1.0"))){
        sdp->disk_if = 0;      // Fake SATA
        tindex = 702;
      }
    }else if(0==strncmp(sdp->host->hostt->name,"ahci",strlen("ahci"))){
        tindex = sdp->host->host_no; // For HDD (1~5)
        switch (tindex) {
            case 0: // disk 1 for EVT board
                tindex = 0;
                break;
            case 1: // disk 1 for DVT board
            case 2: // disk 2
            case 3: // disk 3
            case 4: // disk 4
            case 5: // disk 5
                tindex = tindex - 1;
                break;
	}
    }else if(0==strncmp(sdp->host->hostt->name,"uas",strlen("uas"))){
        sdp->disk_if = 2;      // Let UAS = USB
    }
    return tindex;

}

u32 n4810_disk_access(int index,int act){
    if (index <= 0) return 1;

    //only handle 1~4 disk access
    if (index > 4) return 1;
    
    if (act==LED_DISABLE){
        act=LED_OFF;
    }
  
    if (act==LED_ON)
        SOC_gpio_write_bit(index+27,1);
    else if (act==LED_OFF)
        SOC_gpio_write_bit(index+27,0);

    return 0;
}

u32 n4810_disk_index(int index, struct scsi_device *sdp){
    u32 tindex=index;
    if(0==strncmp(sdp->host->hostt->name,"usb-storage",strlen("usb-storage"))){
      if(0==strncmp(sdp->host->hostt->info(sdp->host),"usb-storage 1-1:1.0",strlen("usb-storage 1-1:1.0"))){
        sdp->disk_if = 0;      // Fake SATA
        tindex = 702;
      }
    }else if(0==strncmp(sdp->host->hostt->name,"ahci",strlen("ahci"))){
        tindex = sdp->host->host_no; // For HDD (1~4)
    }else if(0==strncmp(sdp->host->hostt->name,"uas",strlen("uas"))){
        sdp->disk_if = 2;      // Let UAS = USB
    }
    return tindex;

}

static void reset_pic(void)
{
    u8 val;
    printk("RESET_PIC\n");
    val = 1;
    pca9532_od_set_led(7, val);
    udelay(60);
    val = 0;
    pca9532_od_set_led(7, val);
}

static ssize_t proc_thecus_io_write(struct file *file,
				    const char __user * buf, size_t length,
				    loff_t * ppos)
{
    char *buffer, buf1[20];
    int i, err, v1, v2;
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
     * Usage: echo "SD_LED 0|1|2" >/proc/thecus_io //2:Blink * SD BUSY led
     * Usage: echo "SDF_LED 0|1|2" >/proc/thecus_io //2:Blink * SD ERROR led
     * Usage: echo "LOGO1_LED 0|1|2" >/proc/thecus_io //2:Blink * LOGO1 led
     * Usage: echo "LOGO2_LED 0|1|2" >/proc/thecus_io //2:Blink * LOGO2 led
     * Usage: echo "Fail 0|1" >/proc/thecus_io                  * LED System Fail
     * Usage: echo "Busy 0|1" >/proc/thecus_io                  * LED System Busy
     * Usage: echo "Buzzer 0|1" >/proc/thecus_io                * Buzzer
     * Usage: echo "RESET_PIC" >/proc/thecus_io                * RESET_PIC
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

	    if (v1 >= 1 && v1 <= 12) {
		v1 = v1 - 1;
		pca9532_set_led(v1, val);
	    } else {
		pca9532_set_led(v1, val);
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
	    pca9532_od_set_led(5, val);
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
	    pca9532_od_set_led(6, val);
	}
    } else if (!strncmp(buffer, "PWR_LED", strlen("PWR_LED"))) {
	i = sscanf(buffer + strlen("PWR_LED"), "%d\n", &v1);
	if (i == 1)		//only one input
	{
	    _DBG(1, "PWR_LED %d\n", v1);
	    if (v1 == 0)	//input 0: want to turn off
		val = LED_OFF;
	    else if (v1 == 1)	//turn on
		val = LED_ON;
	    else
		val = LED_BLINK1;

	    led_sys_power = val;
	    pca9532_od_set_led(0, val);
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
	    pca9532_od_set_led(3, val);
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
	    pca9532_od_set_led(4, val);
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
	    pca9532_od_set_led(8, val);
	}
    } else if(!strncmp (buffer, "BAT_CHARG", strlen ("BAT_CHARG"))){
        i = sscanf (buffer + strlen ("BAT_CHARG"), "%d\n",&v1);
        if (i==1){ //only one input
            if(v1==0)//input 0: want to turn off
                val = 1;   
            else
                val = 0;   

	    pca9532_od_set_led(12, val);
        }
    } else if (!strncmp(buffer, "RESET_PIC", strlen("RESET_PIC"))) {
        reset_pic();
    }

    err = length;
  out:
    free_page((unsigned long) buffer);
  out2:
    *ppos = 0;

    return err;
}

static bool battery_model_check(int board_id)
{
	int i;
	for(i=0; i<sizeof(battery_board_index)/sizeof(int); i++){
		if(board_id == battery_board_index[i]){
			return true;
		}
	}
	return false;
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
    seq_printf(m, "SD_LED: %s\n", LED_STATUS[led_sd_busy]);
    seq_printf(m, "SDF_LED: %s\n", LED_STATUS[led_sd_err]);
    seq_printf(m, "LOGO1_LED: %s\n", LED_STATUS[led_logo1]);
    seq_printf(m, "LOGO2_LED: %s\n", LED_STATUS[led_logo2]);
    seq_printf(m, "LED_Power: %s\n", LED_STATUS[led_sys_power]);
    seq_printf(m, "LED_Busy: %s\n", LED_STATUS[led_sys_busy]);
    seq_printf(m, "LED_Fail: %s\n", LED_STATUS[led_sys_err]);

    if (battery_model_check(board_idx)){
        seq_printf(m, "BAT_FN: %d\n", 1);
        seq_printf(m, "BAT_TMODE: BJT\n");

        val = pca9532_id_get_input(13);
        if(val>=0) seq_printf(m,"HAS_BAT: %s\n", val?"HIGH":"LOW");

        val = pca9532_od_get_led(12);
        if(val>=0) seq_printf(m,"BAT_CHARG: %s\n", val?"LOW":"HIGH");

        val = pca9532_id_get_input(12);
        if(val>=0) seq_printf(m,"AC_RDY: %s\n", val?"HIGH":"LOW");
    }else{
        seq_printf(m, "BAT_FN: %d\n", 0);
    }

    seq_printf(m, "CONSOLE: %s\n", "ttyS1");

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

    if (battery_model_check(board_idx)){
        val = pca9532_id_get_input(13);
        if(val == 0){
            val = pca9532_id_get_input(12);
            if(sleepon_flag){
                if(val == 0){
                    if(bat_flag == 1){
                        sprintf(Message, "AC Ready\n");
                        wake_up_interruptible(&thecus_event_queue);
                    }
                    bat_flag=0;
                }else if (val == 1){
                    if(bat_flag == 0){
                        sprintf(Message, "AC Fail\n");
                        wake_up_interruptible(&thecus_event_queue);
                    }
                    bat_flag=1;
                }
            }
        }
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
        pca9532_set_led(13, LED_OFF);
        pca9532_set_led(14, LED_OFF);
        pca9532_id_set_led(9, LED_OFF);
        pca9532_id_set_led(10, LED_OFF);
        pca9532_id_set_led(11, LED_OFF);
        pca9532_id_set_led(12, LED_OFF);
	pca9532_id_set_led(14, LED_OFF);
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

    ret = SOC_GPIO_init();
    if (ret < 0) {
//        printk(KERN_ERR "SOC_GPIO_init failed\n");
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

    val = pca9532_id_get_input(0);
    board_num |= val;
    val = pca9532_id_get_input(1);
    board_num |= (val<<1);
    val = pca9532_id_get_input(2);
    board_num |= (val<<2);
    val = pca9532_id_get_input(3);
    board_num |= (val<<3);

    printk(KERN_INFO "thecus_io: board_num: %Xh\n", board_num);
    for (n = 0; board_info[n].board_string; n++)
        if (board_num == board_info[n].gpio_num) {
            board_idx = n;
            break;
        }

    thecus_board_register(&board_info[board_idx]);

    /* Current ACPI reboot cause H/W reset in bay trail */
    thecus_acpi_reboot=0;

    pca9532_id_set_led(15, 0);

    // turn off busy/err led
    pca9532_id_set_led(9, LED_OFF);
    pca9532_id_set_led(10, LED_OFF);
    pca9532_id_set_led(11, LED_OFF);
    pca9532_id_set_led(12, LED_OFF);
    if (board_idx == 0)
        pca9532_id_set_led(14, LED_OFF);
    pca9532_set_led(13, LED_OFF);
    pca9532_set_led(14, LED_ON);

    // load driver, initial blink busy led
    pca9532_id_set_led(9, LED_BLINK1);

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

    SOC_GPIO_exit();

    unregister_reboot_notifier(&sys_notifier_reboot);

    thecus_board_unregister(&board_info[board_idx]);
}

module_init(thecus_io_init);
module_exit(thecus_io_exit);

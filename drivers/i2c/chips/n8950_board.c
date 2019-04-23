/*
 *  Copyright (C) 2015 Thecus Technology Corp. 
 *
 *      Maintainer: Derek Lin <derek_lin@thecus.com>
 *      Maintainer: Joey Wang <joey_wang@thecus.com>
 *
 *      Driver for Thecus N8950U/N12850U/N16850U board's I/O
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

#include "thecus_board.h"

MODULE_AUTHOR("Derek Lin <derek_lin@thecus.com>;Joey Wang <joey_wang@thecus.com>");
MODULE_DESCRIPTION
    ("Thecus N12850/N16850/N12850L/N12850RU MB Driver and board depend io operation");
MODULE_LICENSE("GPL");

/*
 *  0 : ipmi not implement yet (default)
 *  1 : expander version 1 (2 pca9532)
 *  2 : expander version 2 (3 pca9532)
 */
#define N8950_EXPANDER_VERSION 2

static u32 hw_disk_access(int index, int act);
u32 n8950_disk_index(int index, struct scsi_device *sdp);
u32 n8910_disk_index(int index, struct scsi_device *sdp);

static const struct thecus_function n8910_func = {
	.disk_access = hw_disk_access,
	.disk_index  = n8910_disk_index,
};

static const struct thecus_function n8950_func = {
	.disk_access = hw_disk_access,
	.disk_index  = n8950_disk_index,
};

static int board_idx = 0;
static struct thecus_board board_info [] = {
#if N8950_EXPANDER_VERSION == 2
	{ 0, "N12850"   , 12, 0, 0, "1100", "BOARD_N8950", &n8950_func},
	{ 1, "N16850"   , 16, 0, 0, "1101", "BOARD_N8950", &n8950_func},
	{ 2, "N12910SAS", 12, 0, 0, "1102", "BOARD_N8910", &n8910_func},
	{ 3, "N16910SAS", 16, 0, 0, "1103", "BOARD_N8910", &n8910_func},
	{ 4, "N12910"   , 12, 0, 0, "1104", "BOARD_N8910", &n8910_func},
	{ 5, "N16910"   , 16, 0, 0, "1105", "BOARD_N8910", &n8910_func},
	{ 6, "N8910"    ,  8, 0, 0, "1106", "BOARD_N8910", &n8910_func},
	{ 7, "N12850L"  , 12, 0, 0, "1107", "BOARD_N8950", &n8950_func},
	{ 8, "N12850RU" , 12, 0, 0, "1108", "BOARD_N8950", &n8950_func},
#else
	{ 0, "N8950U" ,  8, 0, 0, "1100", "BOARD_N8950", &n8950_func},
	{ 1, "N12850U", 12, 0, 0, "1101", "BOARD_N8950", &n8950_func},
	{ 2, "N16850U", 16, 0, 0, "1102", "BOARD_N8950", &n8950_func},
#endif
	{ }
};


static char * debug_port[]={"ttyS0","ttyS0","NONE","NONE","NONE","NONE","NONE","ttyS0","ttyS0"};

extern int pch_8_gpio_init(void);
extern void pch_8_gpio_exit(void);
extern u32 thecus_board_register(struct thecus_board *board);
extern u32 thecus_board_unregister(struct thecus_board *board);

#define GET_INPUT_INDEX(led_num) (((led_num > 7) && (led_num < 16)) ? (led_num - 8) : led_num)
#define GET_INPUT_VALUE(reg, led_num) ((reg & (1 << led_num)) >> led_num)

u32 hw_disk_access(int index, int act)
{
        /* disk access led by hw */
        return 0;
}

u32 n8950_disk_index(int index, struct scsi_device *sdp)
{
        u32 tindex=index;

        if(0==strncmp(sdp->host->hostt->name,"usb-storage",strlen("usb-storage"))){
                if(0==strncmp(sdp->host->hostt->info(sdp->host),"usb-storage 1-1.1:1.0",strlen("usb-storage 1-1.1:1.0"))){
                        sdp->disk_if = 0;      /* Fake SATA */
                        tindex = 702;
                }
        }else if(0==strncmp(sdp->host->hostt->name,"Fusion MPT SAS Host",strlen("Fusion MPT SAS Host"))){
                tindex = sdp->tray_id - 1;
        }else if(0==strncmp(sdp->host->hostt->name,"uas",strlen("uas"))){
                sdp->disk_if = 2;      /* Let UAS = USB */
	}else if(0==strncmp(sdp->host->hostt->name,"ahci",strlen("ahci"))){
		tindex = sdp->host->host_no; // For HDD (1~12) (AHCI)
		switch (tindex) {
                        case 10: // disk 7
                        case 11: // disk 8
                                tindex = tindex - 4;
                                break;
                        case 6: // disk 9
                        case 7: // disk 10
                        case 8: // disk 11
                        case 9: // disk 12
                                tindex = tindex + 2;
                                break;
		}

        }
        return tindex;
}

u32 n8910_disk_index(int index, struct scsi_device *sdp)
{
        u32 tindex=index;

        if(0==strncmp(sdp->host->hostt->name,"usb-storage",strlen("usb-storage"))){
                if(0==strncmp(sdp->host->hostt->info(sdp->host),"usb-storage 1-4:1.0",strlen("usb-storage 1-4:1.0"))){
                        sdp->disk_if = 0;      /* Fake SATA */
                        tindex = 702;
                }
        }else if(0==strncmp(sdp->host->hostt->name,"Fusion MPT SAS Host",strlen("Fusion MPT SAS Host"))){
                tindex = sdp->tray_id - 1;
        }else if(0==strncmp(sdp->host->hostt->name,"uas",strlen("uas"))){
                sdp->disk_if = 2;      /* Let UAS = USB */
        }else if(0==strncmp(sdp->host->hostt->name,"ahci",strlen("ahci"))){
                tindex = sdp->host->host_no; // For HDD (1~12) (AHCI)
                switch (tindex) {
                        case 3: // disk 3
                        case 5: // disk 5
                        case 7: // disk 7
                                tindex = tindex - 1;
                                break;
                        case 2: // disk 4
                        case 4: // disk 6
                        case 6: // disk 8
                                tindex = tindex + 1;
                                break;

        	}

        }
        return tindex;
}

/* PCA9532 Address ADDR[2:0]:1000 */
#define DISK_ERR_LED1  0
#define DISK_ERR_LED2  1
#define DISK_ERR_LED3  2
#define DISK_ERR_LED4  3
#define DISK_ERR_LED5  4
#define DISK_ERR_LED6  5
#define DISK_ERR_LED7  6
#define DISK_ERR_LED8  7
#define DISK_ERR_LED9  8
#define DISK_ERR_LED10 9
#define DISK_ERR_LED11 10
#define DISK_ERR_LED12 11
#define DISK_ERR_LED13 12
#define DISK_ERR_LED14 13
#define DISK_ERR_LED15 14
#define DISK_ERR_LED16 15

#if N8950_EXPANDER_VERSION == 2

#define IDEX_MAX 8

/* 
 * PCA9532 Address ADDR[2:0]:010 
 * Input part
 */
#define MBID0     0
#define MBID1     1
#define MBID2     2
#define MBID3     3
#define MBID4     4
#define MBID5     5
#define MBID6     6
#define MBID7     7
#define COPY_BTN  8
#define PSU_FAIL1 9
#define PSU_FAIL2 10
#define CASE_OPEN 11
#define DC_MODE   12
#define BAT_PRSNT 13
#define MUTE      14
#define PCAI_GP15 15

#define INPUT1    1
/*
 * PCA9532 Address ADDR[2:0]:0010
 * Onput part
 */
#define SYS_PWR1     0
#define SYS_PWR2     1
#define SYS_PWR3     2
#define SYS_BUSY     3
#define SYS_ERR      4
#define USB_ACT      5
#define USB_ERR      6
#define PIC_RST      7
#define BUZZER       8
#define LOGO_LED1    9
#define LOGO_LED2    10
#define LCM_DISABLE  11
#define BAT_CHG_DIS  12
#define PCAO_GP13    13
#define PCAO_GP14    14
#define PCAO_GP15    15

extern void pca9532_set_led(int led_num, int led_state);
extern void pca9532_id_set_led(int led_num, int led_state);
extern void pca9532_od_set_led(int led_num, int led_state);
extern int pca9532_id_get_input(int led_num);
extern int pca9532_id_get_whole_input(int input_num, u8 *val);
extern int pca9532_od_get_led(int led_num);
extern int pca9532_od_get_input(int led_num);

#else

#define IDEX_MAX 6

/* PCA9532 Address ADDR[2:0]:010 */
#define MBID0     0
#define MBID1     1
#define MBID2     2
#define MBID3     3
#define MBID4     4
#define MBID5     5
#define MUTE      6
#define PSU_FAIL1 7
#define PCA_GPO8  8
#define PCA_GPO9  9
#define PIC_RST   10
#define USB_ERR   11
#define USB_ACT   12
#define SYS_PWR1  13
#define SYS_BUSY  14
#define SYS_ERR   15

#if N8950_EXPANDER_VERSION == 1
extern void pca9532_set_led(int led_num, int led_state);
extern void pca9532_id_set_led(int led_num, int led_state);
extern int pca9532_id_get_input(int led_num);
#else
static void pca9532_set_led(int led_num, int led_state){return;}
static void pca9532_id_set_led(int led_num, int led_state){return;}
static int pca9532_id_get_input(int led_num){return 1;} /* Low active */
#endif

#endif

static void io_set_disk_err_led(int led_num, int led_state)
{
    pca9532_set_led(led_num, led_state);
}

static int io_get_input(int led_num)
{
    return pca9532_id_get_input(led_num);
}

static int io_get_whole_input(int input_num, u8 *val)
{
    return pca9532_id_get_whole_input(input_num, val);
}

static void io_set_output(int led_num, int led_state)
{
#if N8950_EXPANDER_VERSION == 2
    pca9532_od_set_led(led_num, led_state);
#else
    pca9532_id_set_led(led_num, led_state);
#endif
}

static void io_reset_pic(void)
{
    u8 val = 1;

    printk("RESET_PIC\n");
    io_set_output(PIC_RST, val);
    udelay(60);
    val = 0;
    io_set_output(PIC_RST, val);
}

static u32 buzzer_status = BUZZER_OFF;
static u32 led_usb_busy = LED_OFF;
static u32 led_usb_err = LED_OFF;
static u32 led_sys_power = LED_OFF;
static u32 led_sys_busy = LED_OFF;
static u32 led_sys_err = LED_OFF;
static u32 psu1_status = 1;
static u32 psu2_status = 1;
static u32 mute_status = 1;

static void io_init(void)
{
    /* turn off busy/err led */
    io_set_output(SYS_BUSY, led_sys_busy);
    io_set_output(SYS_ERR, led_sys_err);

    /* load driver, initial blink busy led */
    led_sys_busy=LED_BLINK1;
    io_set_output(SYS_BUSY, led_sys_busy);

    /* buzzer off*/
    kd_mksound(0 , 0);
    buzzer_status = BUZZER_OFF;
}

static int led_convert(int status)
{
    int ret = LED_OFF;

    if (status == 1)
        ret = LED_ON;
    else if (status == 2) 
        ret = LED_BLINK1;

    return ret;
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
     * Usage: echo "S_LED 1-16 0|1|2" >/proc/thecus_io //2:Blink * LED SATA 1-16 ERROR led
     * Usage: echo "U_LED 0|1|2" >/proc/thecus_io //2:Blink * USB BUSY led
     * Usage: echo "UF_LED 0|1|2" >/proc/thecus_io //2:Blink * USB ERROR led
     * Usage: echo "Fail 0|1" >/proc/thecus_io                  * LED System Fail
     * Usage: echo "Busy 0|1" >/proc/thecus_io                  * LED System Busy
     * Usage: echo "Buzzer 0|1" >/proc/thecus_io                * Buzzer
     * Usage: echo "RESET_PIC" >/proc/thecus_io                * RESET_PIC 
     */

    if (!strncmp(buffer, "S_LED", strlen("S_LED"))) {
	i = sscanf(buffer + strlen("S_LED"), "%d %d\n", &v1, &v2);
	if (i == 2)		/* two input */
	{
            val = led_convert(v2);
	    if (v1 >= 1 && v1 <= 16) {
		v1 = v1 - 1;
	        io_set_disk_err_led(v1, val);
	    }
	}
    } else if (!strncmp(buffer, "UF_LED", strlen("UF_LED"))) {
	i = sscanf(buffer + strlen("UF_LED"), "%d\n", &v1);
	if (i == 1) {
	    led_usb_err = led_convert(v1);
	    io_set_output(USB_ERR, led_usb_err);
	}
    } else if (!strncmp(buffer, "U_LED", strlen("U_LED"))) {
	i = sscanf(buffer + strlen("U_LED"), "%d\n", &v1);
	if (i == 1) {
	    led_usb_busy = led_convert(v1);
	    io_set_output(USB_ACT, led_usb_busy);
	}
    } else if (!strncmp(buffer, "PWR_LED", strlen("PWR_LED"))) {
	i = sscanf(buffer + strlen("PWR_LED"), "%d\n", &v1);
	if (i == 1) {
	    led_sys_power = led_convert(v1);
	    io_set_output(SYS_PWR1, led_sys_power);
	}
    } else if (!strncmp(buffer, "Busy", strlen("Busy"))) {
	i = sscanf(buffer + strlen("Busy"), "%d\n", &v1);
	if (i == 1) {
	    led_sys_busy = led_convert(v1);
	    io_set_output(SYS_BUSY, led_sys_busy);
	}
    } else if (!strncmp(buffer, "Fail", strlen("Fail"))) {
	i = sscanf(buffer + strlen("Fail"), "%d\n", &v1);
	if (i == 1) {
	    led_sys_err = led_convert(v1);
	    io_set_output(SYS_ERR, led_sys_err);
	}
    } else if (!strncmp(buffer, "Buzzer", strlen("Buzzer"))) {
	i = sscanf(buffer + strlen("Buzzer"), "%d\n", &v1);
	if (i == 1) {
	    buzzer_status = v1? BUZZER_ON : BUZZER_OFF; 
	    kd_mksound(buzzer_status *440 ,0);
	}
    } else if (!strncmp(buffer, "RESET_PIC", strlen("RESET_PIC"))) {
        io_reset_pic();
    } else if (!strncmp(buffer, "HA_EN", strlen("HA_EN"))) {
        i = sscanf(buffer + strlen("HA_EN"), "%d\n", &v1);
        if (i == 1) {
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
    char LED_STATUS[4][8];

    sprintf(LED_STATUS[LED_ON], "ON");
    sprintf(LED_STATUS[LED_OFF], "OFF");
    sprintf(LED_STATUS[LED_BLINK1], "BLINK");
    sprintf(LED_STATUS[LED_BLINK2], "-");

    seq_printf(m, "MODELNAME: %s\n", board_info[board_idx].board_string);
    seq_printf(m, "FAC_MODE: OFF\n");
    seq_printf(m, "Buzzer: %s\n", buzzer_status ? "ON" : "OFF");
    seq_printf(m, "MAX_TRAY: %d\n", board_info[board_idx].max_tray);
    seq_printf(m, "eSATA_TRAY: %d\n", board_info[board_idx].eSATA_tray);
    seq_printf(m, "eSATA_COUNT: %d\n", board_info[board_idx].eSATA_count);
    seq_printf(m, "WOL_FN: %d\n", 1);
    seq_printf(m, "FAN_FN: %d\n", 1);
    seq_printf(m, "BEEP_FN: %d\n", 1);
    seq_printf(m, "eSATA_FN: %d\n", 0);
    seq_printf(m, "MBTYPE: %s\n", board_info[board_idx].mb_type);
    seq_printf(m, "PSU1_FAIL: %s\n", psu1_status?"OFF":"ON");
    seq_printf(m, "PSU2_FAIL: %s\n", psu2_status?"OFF":"ON");
    seq_printf(m, "U_LED: %s\n", LED_STATUS[led_usb_busy]);
    seq_printf(m, "UF_LED: %s\n", LED_STATUS[led_usb_err]);
    seq_printf(m, "LED_Power: %s\n", LED_STATUS[led_sys_power]);
    seq_printf(m, "LED_Busy: %s\n", LED_STATUS[led_sys_busy]);
    seq_printf(m, "LED_Fail: %s\n", LED_STATUS[led_sys_err]);
    seq_printf(m, "HA_EN: %d\n", thecus_ha_enable);
    seq_printf(m, "CONSOLE: %s\n", debug_port[board_idx]);

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
    u8 psu1_val, psu2_val, mute_val;
    u8 input_val;
    int ret;

    /* Get the whole value of input1 once */
    ret = io_get_whole_input(INPUT1, &input_val);
    if (ret < 0){
        psu1_val = psu1_status; 
        psu2_val = psu2_status; 
        mute_val = mute_status; 
    } else {
        psu1_val = GET_INPUT_VALUE(input_val, GET_INPUT_INDEX(PSU_FAIL1)); 
        psu2_val = GET_INPUT_VALUE(input_val, GET_INPUT_INDEX(PSU_FAIL2));
        mute_val = GET_INPUT_VALUE(input_val, GET_INPUT_INDEX(MUTE));
    }

        if(psu1_status != psu1_val) {
            if(psu1_val == 1) {
                sprintf(Message, "PSU1_FAIL OFF\n");
            } else {
                sprintf(Message, "PSU1_FAIL ON\n");
            }
            psu1_status = psu1_val;
            wake_up_interruptible(&thecus_event_queue);
        }

        if(psu2_status != psu2_val) {
            if(psu2_val == 1) {
                sprintf(Message, "PSU2_FAIL OFF\n");
            } else {
                sprintf(Message, "PSU2_FAIL ON\n");
            }
            psu2_status = psu2_val;
            wake_up_interruptible(&thecus_event_queue);
        }

        if ((mute_val == 0) && (buzzer_status != mute_val)) {
	    kd_mksound(0, 0);
	    buzzer_status = BUZZER_OFF;
        }

    /* If cleanup wants us to die */
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
    interruptible_sleep_on(&thecus_event_queue);
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
        /* turn off busy/err led */
        led_sys_busy=LED_OFF;
        led_sys_err=LED_OFF;
        io_set_output(SYS_BUSY, led_sys_busy);
        io_set_output(SYS_ERR, led_sys_err);
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
    int ret,idex;


    ret = pch_8_gpio_init();
    if(ret < 0) {
        printk(KERN_ERR "pch_8_gpio_init failed!\n");
        return ret;
    }else{
        printk(KERN_INFO "pch_8_gpio_init succeeded!\n");
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

    for (idex = 0; idex < IDEX_MAX; idex++){
        val = io_get_input(idex);
        board_num |= (val<<idex);
    }

    for (n = 0; board_info[n].board_string; n++)
        if (board_num == board_info[n].gpio_num) {
            board_idx = n;
            break;
        }

    printk(KERN_INFO "thecus_io: board_num: %Xh,board_idx: %d\n", board_num,board_idx);

    io_init();

    thecus_board_register(&board_info[board_idx]);

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

    pch_8_gpio_exit();

    unregister_reboot_notifier(&sys_notifier_reboot);

    thecus_board_unregister(&board_info[board_idx]);
}

module_init(thecus_io_init);
module_exit(thecus_io_exit);

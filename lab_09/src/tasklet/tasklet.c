#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/init.h> 
#include <linux/interrupt.h> 
#include <linux/init_task.h> 
#include <linux/ktime.h>


#define KEYBOARD_IRQ 1 
#define KBD_DATA_REG 0x60 
#define KBD_SCANCODE_MASK 0x7f
#define KBD_STATUS_MASK 0x80
#define KEYS_COUNT 83

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Varlamova Ekaterina");


static int id;
char tasklet_data[] = "time tasklet data";
struct tasklet_struct *kb_tasklet; 
struct tasklet_struct *time_tasklet; 
char *keyboard_key[] =
	{
		"[ESC]",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"0",
		"-",
		"=",
		"bs",
		"[Tab]",
		"Q",
		"W",
		"E",
		"R",
		"T",
		"Y",
		"U",
		"I",
		"O",
		"P",
		"[",
		"]",
		"[Enter]",
		"[CTRL]",
		"A",
		"S",
		"D",
		"F",
		"G",
		"H",
		"J",
		"K",
		"L",
		";",
		"\'",
		"`",
		"[LShift]",
		"\\",
		"Z",
		"X",
		"C",
		"V",
		"B",
		"N",
		"M",
		",",
		".",
		"/",
		"[RShift]",
		"[PrtSc]",
		"[Alt]",
		" ", // Space
		"[Caps]",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"[Num]",
		"[Scroll]",
		"[Home(7)]",
		"[Up(8)]",
		"[PgUp(9)]",
		"-",
		"[Left(4)]",
		"[Center(5)]",
		"[Right(6)]",
		"+",
		"[End(1)]",
		"[Down(2)]",
		"[PgDn(3)]",
		"[Ins]",
		"[Del]",
}; 
void kb_tasklet_handler(unsigned long data) 
{
	printk(KERN_INFO "KEYBOARD TASKLET: state - %ld, count - %d, data - %d\n",
		kb_tasklet->state, kb_tasklet->count, kb_tasklet->data);
	int scancode = kb_tasklet->data;
	
	if (scancode & KBD_STATUS_MASK)
	{
		scancode &= KBD_SCANCODE_MASK;
		if (scancode > KEYS_COUNT)
			printk("KEYBOARD TASKLET: there is no such keyboard key");
		else
			printk("KEYBOARD TASKLET: keyboard key %s is released", keyboard_key[scancode - 1]);
	} else
	{
		scancode &= KBD_SCANCODE_MASK;
		if (scancode > KEYS_COUNT)
			printk("KEYBOARD TASKLET: there is no such keyboard key");
		else
			printk("KEYBOARD TASKLET: keyboard key %s is pressed", keyboard_key[scancode - 1]);
	}
}

void time_tasklet_handler(unsigned long data) 
{
	struct timespec64 tm;
	ktime_get_real_ts64(&tm);
	tm.tv_sec += 3 * 3600;
	printk(KERN_INFO "TIME TASKLET: state - %ld, count - %d, data - %s, time - %.2llu:%.2llu:%.2llu\n",
		time_tasklet->state, 
		time_tasklet->count, 
		time_tasklet->data,
		(tm.tv_sec / 3600) % (24),
                (tm.tv_sec / 60) % (60),
                tm.tv_sec % 60);
}


static irqreturn_t interrupt_handler(int irq, void *dev_id) 
{
	if (irq == KEYBOARD_IRQ) 
	{
		int scancode = inb(KBD_DATA_REG);
		printk(KERN_INFO "TASKLET: Keyboard tasklet scheduled\n");
		tasklet_init(kb_tasklet, kb_tasklet_handler, (unsigned long) scancode);
		tasklet_schedule(kb_tasklet);
		
		tasklet_schedule(time_tasklet);
		return IRQ_HANDLED;
	} 
	return IRQ_NONE;
}

static int __init mod_init(void)
{
	
	if (request_irq(KEYBOARD_IRQ, interrupt_handler, IRQF_SHARED, "interrupt_with_tasklet", &id))
	{
		printk(KERN_ERR "TASKLET: Error on request_irq\n"); 
		return -1;
	}
 
 	kb_tasklet  = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
	if (kb_tasklet == NULL) {
		printk(KERN_ERR "TASKLET: cannot allocate Memory");
		return -1;
	}
	
	time_tasklet  = kmalloc(sizeof(struct tasklet_struct), GFP_KERNEL);
	if (time_tasklet == NULL) {
		kfree(kb_tasklet);
		printk(KERN_ERR "TASKLET: cannot allocate Memory");
		return -1;
	}
	tasklet_init(time_tasklet, time_tasklet_handler, (unsigned long) &tasklet_data);
	
	printk(KERN_INFO "TIME TASKLET: count - %d\n", time_tasklet->count);
	tasklet_disable(time_tasklet);
	printk(KERN_INFO "TIME TASKLET: count - %d\n", time_tasklet->count);
        tasklet_enable(time_tasklet);

	printk(KERN_INFO "TASKLET: Module loaded!\n");
	return 0;
}

static void __exit mod_exit(void) 
{
	tasklet_kill(kb_tasklet);
	kfree(kb_tasklet);
	
	tasklet_kill(time_tasklet);
	kfree(time_tasklet);
	
	free_irq(KEYBOARD_IRQ, &id); 
	printk(KERN_INFO "TASKLET: Module unloaded!\n");
}

module_init(mod_init);
module_exit(mod_exit); 

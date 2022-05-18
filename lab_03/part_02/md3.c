#include <linux/init_task.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "md.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varlamova Ekaterina");


static int __init md_init(void) {
    
    printk("Module md3 was loaded\n");
    printk("Value exported from md1 : %s\n", md1_data);
    printk("Value returned from function md1_proc is : %s\n", md1_proc());
    return -1;
}

static void __exit md_exit(void) 
{ 
	printk("Module md3 was unloaded\n"); 
}


module_init(md_init);
module_exit(md_exit);

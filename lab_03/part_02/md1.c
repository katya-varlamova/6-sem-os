#include <linux/init_task.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include "md.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varlamova Ekaterina");

char *md1_data = "md1_var_data";

char *md1_proc(void)
{
	return md1_data;
}
char *md1_proc_noexport(void)
{
	return md1_data;
}

static char *md1_proc_local(void)
{
	return md1_data;
}
EXPORT_SYMBOL(md1_data);
EXPORT_SYMBOL(md1_proc);

static int __init md_init(void) {
    
    printk("Module md1 was loaded\n");
    return 0;
}

static void __exit md_exit(void) 
{ 
	printk("Module md1 was unloaded\n"); 
}

module_init(md_init);
module_exit(md_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varlamova Ekaterina");

#define DIRNAME "fortune_dir"
#define FILENAME "fortune"
#define SYMLINK "fortune_ln"
#define FILEPATH DIRNAME "/" FILENAME

static struct proc_dir_entry *fortune_dir = NULL;
static struct proc_dir_entry *fortune = NULL;
static struct proc_dir_entry *fortune_ln = NULL;

static char *cookie_buffer;
static int next_fortune;
static int cookie_index;
static char tmp[PAGE_SIZE];

ssize_t fortune_read(struct file *filp, char __user *buf, size_t count, loff_t *offp)
{
    int len;

    printk(KERN_DEBUG "fortune: read operation called\n");

    if (*offp > 0 || !cookie_index)
    {
        printk(KERN_DEBUG "fortune: data hasn't been written yet");
        return 0;
    }

    if (next_fortune >= cookie_index)
        next_fortune = 0;

    len = snprintf(tmp, PAGE_SIZE, "%s", &cookie_buffer[next_fortune]);
    if (copy_to_user(buf, tmp, len + 1))
    {
        printk(KERN_ERR "fortune: copy_to_user error\n");
        return -EFAULT;
    }

    next_fortune += len + 1;
    *offp += len + 1;

    return len;
}

ssize_t fortune_write(struct file *filp, const char __user *buf, size_t len, loff_t *offp)
{

    printk(KERN_DEBUG "fortune: write operation called\n");
    
    if (len + 1 > PAGE_SIZE - cookie_index)
    {
        printk(KERN_ERR "fortune: cookie_buffer overflow error\n");
        return -ENOSPC;
    }

    if (copy_from_user(&cookie_buffer[cookie_index], buf, len))
    {
        printk(KERN_ERR "fortune: copy_to_user error\n");
        return -EFAULT;
    }

    cookie_index += len;
    cookie_buffer[cookie_index++] = '\0';

    return len;
}

int fortune_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fortune: called open\n");
    return 0;
}

int fortune_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fortune: called release\n");
    return 0;
}

static struct proc_ops fops = {
    .proc_read = fortune_read,
    .proc_write = fortune_write,
    .proc_open = fortune_open,
    .proc_release = fortune_release};

static void freemem(void)
{
    if (fortune_ln)
        remove_proc_entry(SYMLINK, NULL);

    if (fortune)
        remove_proc_entry(FILENAME, fortune_dir);

    if (fortune_dir)
        remove_proc_entry(DIRNAME, NULL);

    if (cookie_buffer)
        vfree(cookie_buffer);
}

static int __init fortune_init(void)
{
    if (!(cookie_buffer = vmalloc(PAGE_SIZE)))
    {
        freemem();
        printk(KERN_ERR "fortune: error during vmalloc\n");
        return -ENOMEM;
    }

    memset(cookie_buffer, 0, PAGE_SIZE);

    if (!(fortune_dir = proc_mkdir(DIRNAME, NULL)))
    {
        freemem();
        printk(KERN_ERR "fortune: error during directory creation\n");
        return -ENOMEM;
    }

    if (!(fortune = proc_create(FILENAME, 0666, fortune_dir, &fops)))
    {
        freemem();
        printk(KERN_ERR "fortune: error during file creation\n");
        return -ENOMEM;
    }

    if (!(fortune_ln = proc_symlink(SYMLINK, NULL, FILEPATH)))
    {
        freemem();
        printk(KERN_ERR "fortune: error during symlink creation\n");
        return -ENOMEM;
    }

    next_fortune = 0;
    cookie_index = 0;

    printk(KERN_INFO "fortune: module loaded\n");

    return 0;
}

static void __exit fortune_exit(void)
{
    freemem();
    printk(KERN_INFO "fortune: module unloaded\n");
}

module_init(fortune_init)
module_exit(fortune_exit)

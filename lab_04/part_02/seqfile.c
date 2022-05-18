#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varlamova Ekaterina");


#define DIRNAME "seqfile_dir"
#define FILENAME "seqfile"
#define SYMLINK "seqfile_ln"
#define FILEPATH DIRNAME "/" FILENAME

static struct proc_dir_entry *fortune_dir = NULL;
static struct proc_dir_entry *fortune = NULL;
static struct proc_dir_entry *fortune_ln = NULL;

static char *cookie_buffer;
static int next_fortune;
static int cookie_index;

static char tmp[PAGE_SIZE];


int fortune_show(struct seq_file *m, void *v)
{
    int len;

    if (!cookie_index)
    {
        return 0;
    }
    
    if (next_fortune >= cookie_index)
        next_fortune = 0;

    len = snprintf(tmp, PAGE_SIZE, "%s", &cookie_buffer[next_fortune]);
    seq_printf(m, "%s", tmp);
    next_fortune += len + 1;
    printk(KERN_INFO "sequence: show is called\n");

    return 0;
}

ssize_t fortune_write(struct file *filp, const char __user *buf, size_t len, loff_t *offp)
{

    printk(KERN_DEBUG "sequence: write operation called\n");
    
    if (len + 1 > PAGE_SIZE - cookie_index)
    {
        printk(KERN_ERR "sequence: cookie_buffer overflow error\n");
        return -ENOSPC;
    }

    if (copy_from_user(&cookie_buffer[cookie_index], buf, len))
    {
        printk(KERN_ERR "sequence: copy_to_user error\n");
        return -EFAULT;
    }

    cookie_index += len;
    cookie_buffer[cookie_index++] = '\0';

    return len;
}

int fortune_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "sequence: called open\n");
    return single_open(file, fortune_show, NULL);
}

int fortune_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "sequence: called release\n");
    return single_release(inode, file);
}

static struct proc_ops fops = {
    .proc_read = seq_read,
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
        printk(KERN_ERR "sequence: error during vmalloc\n");
        return -ENOMEM;
    }

    memset(cookie_buffer, 0, PAGE_SIZE);

    if (!(fortune_dir = proc_mkdir(DIRNAME, NULL)))
    {
        freemem();
        printk(KERN_ERR "sequence: error during directory creation\n");
        return -ENOMEM;
    }
    else if (!(fortune = proc_create(FILENAME, 0666, fortune_dir, &fops)))
    {
        freemem();
        printk(KERN_ERR "sequence: error during file creation\n");
        return -ENOMEM;
    }
    else if (!(fortune_ln = proc_symlink(SYMLINK, NULL, FILEPATH)))
    {
        freemem();
        printk(KERN_ERR "sequence: error during symlink creation\n");
        return -ENOMEM;
    }

    cookie_index = 0;
    next_fortune = 0;

    printk(KERN_INFO "sequence: module loaded\n");

    return 0;
}

static void __exit fortune_exit(void)
{
    freemem();
    printk(KERN_INFO "sequence: module unloaded\n");
}

module_init(fortune_init)
module_exit(fortune_exit)

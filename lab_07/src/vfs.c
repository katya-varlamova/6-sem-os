#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varlamova Ekaterina");
MODULE_DESCRIPTION("myfs");

#define MYFS_MAGIC_NUMBER 0x13131313
#define SLABNAME "myfs_inode_cache"

struct myfs_inode
{
	umode_t i_mode;
	unsigned long i_ino;
};

static int busy = 0;
static struct kmem_cache *myfs_inode_cachep = NULL;
static struct myfs_inode **myfs_inodes = NULL;
static int cache_inode_count = 128;
module_param(cache_inode_count, int, 0);


static struct myfs_inode *cache_get_inode(void)
{
	if (busy == cache_inode_count)
		return NULL;

	return myfs_inodes[busy++] = kmem_cache_alloc(myfs_inode_cachep, GFP_KERNEL);
}

static struct inode *myfs_make_inode(struct super_block *sb, int mode)
{
	printk(KERN_DEBUG "myfs: make inode called\n");
	struct myfs_inode *myfs_inodep = NULL;
	struct inode *ret = new_inode(sb);
	if (ret)
	{
		inode_init_owner(&init_user_ns, ret, NULL, mode);
		ret->i_size = PAGE_SIZE;
		ret->i_atime = ret->i_mtime = ret->i_ctime = current_time(ret);
		if ((myfs_inodep = cache_get_inode()))
		{
			myfs_inodep->i_mode = ret->i_mode;
			myfs_inodep->i_ino = ret->i_ino;
		}
		ret->i_private = myfs_inodep;
		ret->i_ino = 1;
	}

	return ret;
}

static void myfs_put_super(struct super_block *sb)
{
	printk(KERN_DEBUG "myfs: put super called\n");
}

static const struct super_operations myfs_super_ops = {
	.put_super = myfs_put_super,
	.statfs = simple_statfs,
	.drop_inode = generic_delete_inode,
};

static int myfs_fill_sb(struct super_block *sb, void *data, int silent)
{
	printk(KERN_DEBUG "myfs: fill sb called\n");
	struct inode *root = NULL;

	sb->s_blocksize = PAGE_SIZE;
	sb->s_blocksize_bits = PAGE_SHIFT;
	sb->s_magic = MYFS_MAGIC_NUMBER;
	sb->s_op = &myfs_super_ops;

	if (!(root = myfs_make_inode(sb, S_IFDIR | 0755)))
	{
		printk(KERN_ERR "myfs: inode allocation failed\n");
		return -ENOMEM;
	}

	root->i_op = &simple_dir_inode_operations;
	root->i_fop = &simple_dir_operations;

	if (!(sb->s_root = d_make_root(root)))
	{
		iput(root);
		printk(KERN_ERR "myfs: root creation failed\n");
		return -ENOMEM;
	}

	return 0;
}

static struct dentry *myfs_mount(struct file_system_type *type, int flags, const char *dev, void *data)
{
	printk(KERN_DEBUG "myfs: mount called\n");
	struct dentry *entry = mount_nodev(type, flags, data, myfs_fill_sb);
	if (IS_ERR(entry))
		printk(KERN_ERR "myfs: mounting failed\n");
	else
		printk(KERN_DEBUG "myfs: mount succeeded\n");

	// вернуть корневой каталог
	return entry;
}

void kill_sb(struct super_block *sb)
{
	printk(KERN_DEBUG "myfs: kill sb called\n");
	kill_anon_super(sb);
}

static struct file_system_type myfs_type = {
	.owner = THIS_MODULE,
	.name = "myfs",
	.mount = myfs_mount,
	.kill_sb = kill_sb,
};

static int __init myfs_init(void)
{
	printk(KERN_DEBUG "myfs: module init\n");
	int ret = register_filesystem(&myfs_type);
	if (ret)
	{
		printk(KERN_ERR "myfs: cannot register filesystem\n");
		return ret;
	}

	if (!(myfs_inodes = kmalloc(sizeof(struct myfs_inode *) * cache_inode_count, GFP_KERNEL)))
	{
		printk(KERN_ERR "myfs: kmalloc error\n");
		return -ENOMEM;
	}

	if (!(myfs_inode_cachep = kmem_cache_create(SLABNAME, sizeof(struct myfs_inode), 0, SLAB_POISON, NULL)))
	{
		kfree(myfs_inodes);
		printk(KERN_ERR "myfs: kmem_cache_create error\n");
		return -ENOMEM;
	}
	return 0;
}

static void __exit myfs_exit(void)
{
	printk(KERN_DEBUG "myfs: module exit\n");
	int i;
	for (i = 0; i < busy; ++i)
		kmem_cache_free(myfs_inode_cachep, myfs_inodes[i]);

	kmem_cache_destroy(myfs_inode_cachep);
	kfree(myfs_inodes);

	if (unregister_filesystem(&myfs_type))
		printk(KERN_ERR "myfs: cannot unregister filesystem\n");

}

module_init(myfs_init)
module_exit(myfs_exit)

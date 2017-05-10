/* 
 * char_drv.c: Creates a simple char device in which you can write to and
 * read from.
 * */

#include <linux/module.h>	/* Required by all modules (contain init and cleanup macros def) */
#include <linux/fs.h>     	/* Contains data structures required by devices file, inode...  */
#include <asm/uaccess.h>	/* Required to move data to and from user space to kernel space */
#include <linux/cdev.h>		/* cdev definitions, cdev_init, add, del... */

/* Prototypes */
static int __init char_drv_init(void);
static void __exit char_drv_cleanup(void);
static int char_drv_open(struct inode *inode, struct file *filp);
static int char_drv_release(struct inode *inode, struct file *filp);
static ssize_t char_drv_read(struct file *filp, char *buf, size_t count, loff_t *ppos);
static ssize_t char_drv_write(struct file *flip, const char *buf, size_t count, loff_t *ppos);

/* Definitions */
#define DEVICE_NAME "char_drv"		/* Name that will appear under /proc/devices */
#define BUF_LEN 80			/* MAX length of the message */

/* Global variables */
static dev_t device_numbers;	/* Will hold the major and minor numbers assigned to the driver */
static int kern_buf[BUF_LEN];	/* Kernel data buffer */
static int kern_buf_pos = 0;	/* Variable to keep track of buffer position */
static int kern_buf_pos_rd = 0; /* Variable to keep track of read buffer */

/* File operations for this device */
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.read = char_drv_read,
	.write = char_drv_write,
	.open = char_drv_open,
	.release = char_drv_release,
};

/* Structure that represents the device */
static struct cdev char_drv_cdev;

/*
 * This function is called when the module is loaded and it registers the device
 * */
static int __init char_drv_init(void){
	int err = 0;
	
	/* Getting major and minor region for character driver */
	err = alloc_chrdev_region(&device_numbers, 0, 1, DEVICE_NAME);
	if(err < 0){
		printk(KERN_ALERT "Unable to register %s, with errno %d",DEVICE_NAME, err);
		goto fail1;
	}

	/* Creating character device (cdev) "object" */
	cdev_init(&char_drv_cdev, &fops);
	char_drv_cdev.owner = THIS_MODULE;
	char_drv_cdev.ops = &fops;
	err = cdev_add(&char_drv_cdev, device_numbers, 1);
	if(err){
		printk(KERN_ALERT "Unable to add cdev, with errno %d", err);
		goto fail2;
	}
	
	printk(KERN_INFO "char_drv successfully registered with major number %d\n", MAJOR(device_numbers));
	printk(KERN_INFO "to talk to the driver create a dev file with\n");
	printk(KERN_INFO "mknod /dev/%s c %d 0\n", DEVICE_NAME, MAJOR(device_numbers));
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");
	return err;

	fail1:
		return err;
	fail2:
		unregister_chrdev_region(device_numbers, 1);
		return err;
}

/*
 * Cleanup function for char_drv
 * */
static void __exit char_drv_cleanup(void){
	
	cdev_del(&char_drv_cdev);
	unregister_chrdev_region(device_numbers, 1);
	printk(KERN_INFO "%s unregistered\n", DEVICE_NAME);
}


static int char_drv_open(struct inode *inode, struct file *filp){
	return 0;
}

static int char_drv_release(struct inode *inode, struct file *filp){
	/* Reset buf read position */
	kern_buf_pos_rd = 0;
	return 0;
}

static ssize_t char_drv_read(struct file *filp, char *buf, size_t count, loff_t *ppos){
	
	int bytes_not_copied = 0;
	/* Check if process already read all data in buffer */
	if(kern_buf_pos_rd == kern_buf_pos){
		/* No more data to be read */
		return 0;
	}

	/* Check that count does not exceed current buffer size */
	if(count > kern_buf_pos){
		/* Set count to current buffer size */
		count = kern_buf_pos;
	}

	/* Send data in buffer to user space */
	bytes_not_copied = copy_to_user(buf, kern_buf, count);
	kern_buf_pos_rd = count - bytes_not_copied;
	return count - bytes_not_copied;
}

static ssize_t char_drv_write(struct file *flip, const char *buf, size_t count, loff_t *ppos){
	
	/* Check if data to be written is bigger than buffer */
	if(count > (BUF_LEN - kern_buf_pos)){
		/* write up until the max size of the buffer */
		count = BUF_LEN - kern_buf_pos;
		/* Check if there is space in the buffer */
		if(count == 0){
			printk(KERN_INFO "[%s]: kern_buf full!!\n", DEVICE_NAME);
			return -EFAULT;
		}
	}

	/* Copy data from user space to buffer */
	if(copy_from_user((kern_buf + kern_buf_pos), buf, count)){
		/* copy_from_user returns the number of bytes that could not be copied
		 * and zero if it could copy everything
		 * */
		printk(KERN_INFO " Data copy from user space failed!\n");
		return -EFAULT;
	}
	
	kern_buf_pos += count;
	return count;
}

module_init(char_drv_init);
module_exit(char_drv_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Manuel Rodriguez");

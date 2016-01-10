/* Declare what kind of code we want from the header files
Defining __KERNEL__ and MODULE allows us to access kernel-level 
code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/init.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>


MODULE_LICENSE("GPL");

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80		/* Max length of the message from the device */

static int Major;		/* Major number assigned to our device driver */
static char msg[BUF_LEN];	/* The msg the device will give when asked */
static char *msg_Ptr;
static int keySize;
struct file* cipherFile;
struct file* keyFile;

/* Called when module is loaded. 
* Initialize the module - Register the character device */


/*
* This function is called when the module is loaded
*/
int init_module(void)
{
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0) {
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}

	keyFile=NULL;
	cipherFile=NULL;

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return SUCCESS;
}

/*
* This function is called when the module is unloaded
*/
void cleanup_module(void)
{
	unregister_chrdev(Major, DEVICE_NAME);
}

static int device_open(struct inode *inode, struct file *file)
{

	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static long device_ioctl(struct file* f,unsigned int cmd,unsigned long arg){
	if (cmd == 1){
		mm_segment_t oldfs =get_fs();
		set_fs(get_ds());
		struct file* cipher=filp_open((char*)arg,O_RDWR, 0777);
		set_fs(oldfs);
		put_user(cipher,cipherFile);
	}
	if (cmd == 2){
		mm_segment_t oldfs =get_fs();

		set_fs(get_ds());
		struct file* key=filp_open((char*)arg,O_RDONLY,0777);
		set_fs(oldfs);
		put_user(key,keyFile);
	}
}


/* 
* Called when a process closes the device file.
*/
static int device_release(struct inode *inode, struct file *file)
{

/* 
 * Decrement the usage count, or else once you opened the file, you'll
 * never get get rid of the module. 
 */
 mm_segment_t oldfs =get_fs();

 set_fs(get_ds());
 filp_close(keyFile,NULL);
 set_fs(oldfs);
 oldfs =get_fs();
 set_fs(get_ds());
 filp_close(cipherFile,NULL);
 set_fs(oldfs);

 module_put(THIS_MODULE);

 return SUCCESS;
}

static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
		   char *buffer,	/* buffer to fill with data */
		   size_t length,	/* length of the buffer     */
loff_t * offset)
{
/*
 * Number of bytes actually written to the buffer 
 */
 int bytes_read = 0;
 char cipher[2];
 char key[2];
 int i=0;
 int ret;
 int off;
 key[1]='\0';
 cipher[1]='\0';
 if(keyFile== NULL || cipherFile==NULL){
 	return -EINVAL;
 }
 mm_segment_t oldfs;
 while(i<length){
 	oldfs =get_fs();
 	set_fs(get_ds());
 	ret = vfs_read(cipherFile,&cipher,1);
 	set_fs(oldfs);
 	if(ret!=1){
 		printk(KERN_ALERT "Error reading cipher, returned %d\n",ret);
 		break;
 	}
 	off=i % keySize;
 	oldfs =get_fs();
 	set_fs(get_ds());
 	ret = vfs_llseek(keyFile,off,SEEK_SET);
 	set_fs(oldfs);
 	if(ret!=0){
 		printk(KERN_ALERT "Error seeking, returned %d\n",ret);
 		break;
 	}

 	oldfs =get_fs();
 	set_fs(get_ds());
 	ret = vfs_read(keyFile,&key,1);
 	set_fs(oldfs);
 	if(ret!=1){
 		printk(KERN_ALERT "Error reading key, returned %d\n",ret);
 		break;
 	}

 	if(cipher == '\0'){
 		printk("Reached EOF");
 		buffer[i]='\0';
 		break;
 	}
 	buffer[i]=key ^ cipher;
 	i++;
 	bytes_read++;
 }

 return bytes_read;
}

static ssize_t
device_write(struct file *file,
	const char __user * buffer, size_t length, loff_t * offset)
{
	int bytes_read = 0;
	char cipher[2];
	char key[2];
	int i=0;
	int ret;
	int off;
	key[1]='\0';
	cipher[1]='\0';
	if(keyFile== NULL || cipherFile==NULL){
		return -EINVAL;
	}
	mm_segment_t oldfs;
	while(i<length){
		off=i % keySize;
		oldfs =get_fs();
		set_fs(get_ds());
		ret = vfs_llseek(keyFile,off,SEEK_SET);
		set_fs(oldfs);
		if(ret!=1){
			printk(KERN_ALERT "Error seeking, returned %d\n",ret);
			break;
		}

		oldfs =get_fs();
		set_fs(get_ds());
		ret = vfs_read(keyFile,&key,1);
		set_fs(oldfs);
		if(ret!=1){
			printk(KERN_ALERT "Error reading key, returned %d\n",ret);
			break;
		}

		if(buffer[i] == '\0'){
			ret=vfs_write(cipherFile,'\0',1);
			if(ret!=1){
				printk(KERN_ALERT "Error writing cipher, returned %d\n",ret);
				break;
			}
			break;
		}
		ret=vfs_write(cipherFile,buffer[i] ^ key,1);
		if(ret!=1){
			printk(KERN_ALERT "Error writing cipher, returned %d\n",ret);
			break;
		}
		i++;
		bytes_read++;
	}

	return bytes_read;
}


static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.unlocked.ioctl= device_ioctl
};
module_init(init_module);
module_exit(cleanup_module);

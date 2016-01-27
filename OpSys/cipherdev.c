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
#include <linux/slab.h>


#define SUCCESS 0
#define DEVICE_RANGE_NAME "char_dev"
#define DEVICE_FILE_NAME "simple_char_dev"


MODULE_LICENSE("GPL");

struct chardev_info{
	spinlock_t lock;
};

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
/* used to prevent concurent access into the same device */
static struct chardev_info device_info;


static int Major;		/* Major number assigned to our device driver */

/* Called when module is loaded. 
* Initialize the module - Register the character device */



typedef struct pData{
	struct file* keyFile;
	struct file* cipherFile;
	char* keyFileName;
	char* cipherFileName;
	loff_t keySize;
	loff_t cipherSize;
}pData;


long modLong(long a,long b){
	printk("Doing mod of %ld and %ld which is %ld\n",a,b,a-b*(a/b));
	return (a-b*(a/b));
}

static int device_open(struct inode *inode, struct file *file)
{	pData* data=NULL;
	mm_segment_t oldfs;
	struct file *keyFile;
	struct file *cipherFile;
	if(file->private_data==NULL){
		
		data=(pData*) kmalloc(sizeof(pData),GFP_KERNEL);
		data->keyFile=NULL;
		data->cipherFile=NULL;
		data->keyFileName=NULL;
		data->cipherFileName=NULL;
		data->keySize=0;
		data->cipherSize=0;
		file->private_data=data;
	}
	else if(((pData*)(file->private_data))->keyFile==NULL || ((pData*)(file->private_data))->cipherFile==NULL){
		printk("returning 0, one of the files is null\n");
		return 0;
	}
	else{
		oldfs = get_fs();
		set_fs(get_ds());
		keyFile = filp_open(((pData*)(file->private_data))->keyFileName,O_RDONLY,0777);
		set_fs(oldfs);
		if(IS_ERR(keyFile)){
			printk("Error opening key file, error code is %ld\n",PTR_ERR(keyFile));
			return -1;
		}


		oldfs = get_fs();
		set_fs(get_ds());
		cipherFile = filp_open(((pData*)(file->private_data))->cipherFileName,O_RDWR | O_APPEND,0777);
		set_fs(oldfs);
		if(IS_ERR(cipherFile)){
			printk("Error opening cipher file, error code is %ld\n",PTR_ERR(cipherFile));
			return -1;
		}
	}
	return 0;
}


/* 
* Called when a process closes the device file.
*/
static int device_release(struct inode *inode, struct file *file)
{

	mm_segment_t oldfs =get_fs();

	set_fs(get_ds());
	filp_close(((pData*)(file->private_data))->keyFile,NULL);
	set_fs(oldfs);
	oldfs =get_fs();
	set_fs(get_ds());
	filp_close(((pData*)(file->private_data))->cipherFile,NULL);
	set_fs(oldfs);
	kfree(((pData*)(file->private_data))->keyFileName);
	kfree(((pData*)(file->private_data))->cipherFileName);
	kfree(file->private_data);
	return 0;
}

static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
		   char __user *buffer,	/* buffer to fill with data */
		   size_t length,	/* length of the buffer     */
loff_t * offset)
{
/*
 * Number of bytes actually written to the buffer 
 */
 mm_segment_t oldfs;
 struct file* keyFile;
 struct file* cipherFile;
 loff_t keySize;
 int bytes_read = 0;
 char __user cipher[2];
 char __user key[2];
 loff_t i=0;
 int ret;
 loff_t off;
 key[1]='\0';
 cipher[1]='\0';
 cipher[0]=0;


 keyFile=((pData*)(filp->private_data))->keyFile;
 cipherFile=((pData*)(filp->private_data))->cipherFile;
 keySize=((pData*)(filp->private_data))->keySize;

 if(keyFile == NULL || cipherFile == NULL){
 	return -EINVAL;
 }
 printk("The len is %d\n",length);
 while(i<length){
 	printk("cipher pos is %ld\n",cipherFile->f_pos);
 	oldfs =get_fs();
 	set_fs(get_ds());
 	ret = vfs_read(cipherFile,cipher,1,&(cipherFile->f_pos));
 	set_fs(oldfs);
 	if(ret!=1){
 		printk(KERN_ALERT "Error reading cipher, returned %d\n",ret);
 		break;
 	}
 	
 	off=modLong((long)i,(long)keySize);
 	printk("off is: %ld\n",off);
 	oldfs =get_fs();
 	set_fs(get_ds());
 	ret = vfs_llseek(keyFile,off,SEEK_SET);
 	set_fs(oldfs);
 	if(ret!=off){
 		printk(KERN_ALERT "Error seeking, returned %d\n",ret);
 		break;
 	}

 	oldfs =get_fs();
 	set_fs(get_ds());
 	ret = vfs_read(keyFile,key,1,&(keyFile->f_pos));
 	set_fs(oldfs);
 	if(ret!=1){
 		printk(KERN_ALERT "Error reading key, returned %d\n",ret);
 		break;
 	}

 	if(cipher[0] == '\0'){
 		printk("Reached EOF\n");
 		
 		put_user('\0',&buffer[i]);
 		break;
 	}
 	
 	put_user(key[0] ^ cipher[0],&buffer[i]);
 	printk("key is %c and cipher is %x, the out is %x\n",key[0],cipher[0],key[0] ^ cipher[0]);
 	i++;
 	bytes_read++;
 }
 printk("in read, returning %d\n",bytes_read);
 return bytes_read;
}

static ssize_t
device_write(struct file *file,
	const char __user * buffer, size_t length, loff_t * offset)
{
	// int bytes_read = 0;
	// char cipher[2];
	// char key[2];
	// int i=0;
	// int ret;
	// int off;
	// key[1]='\0';
	// cipher[1]='\0';
	// if(keyFile== NULL || cipherFile==NULL){
	// 	return -EINVAL;
	// }
	// mm_segment_t oldfs;
	// while(i<length){
	// 	off=i % keySize;
	// 	oldfs =get_fs();
	// 	set_fs(get_ds());
	// 	ret = vfs_llseek(keyFile,off,SEEK_SET);
	// 	set_fs(oldfs);
	// 	if(ret!=1){
	// 		printk(KERN_ALERT "Error seeking, returned %d\n",ret);
	// 		break;
	// 	}

	// 	oldfs =get_fs();
	// 	set_fs(get_ds());
	// 	ret = vfs_read(keyFile,&key,1);
	// 	set_fs(oldfs);
	// 	if(ret!=1){
	// 		printk(KERN_ALERT "Error reading key, returned %d\n",ret);
	// 		break;
	// 	}

	// 	if(buffer[i] == '\0'){
	// 		ret=vfs_write(cipherFile,'\0',1);
	// 		if(ret!=1){
	// 			printk(KERN_ALERT "Error writing cipher, returned %d\n",ret);
	// 			break;
	// 		}
	// 		break;
	// 	}
	// 	ret=vfs_write(cipherFile,buffer[i] ^ key,1);
	// 	if(ret!=1){
	// 		printk(KERN_ALERT "Error writing cipher, returned %d\n",ret);
	// 		break;
	// 	}
	// 	i++;
	// 	bytes_read++;
	// }

	// return bytes_read;
	mm_segment_t oldfs;
	int num_of_bytes_written = 0;
	char __user cipherBuffer[2];
	char __user keyBuffer[2];
	loff_t i=0;
	int retVal;
	loff_t off;
	struct file* keyFile;
	struct file* cipherFile;
	loff_t keySize;
	size_t numToWrite=1;
	keyBuffer[1]='\0';
	cipherBuffer[1]='\0';

	keyFile=((pData*)(file->private_data))->keyFile;
	cipherFile=((pData*)(file->private_data))->cipherFile;
	keySize=((pData*)(file->private_data))->keySize;

	if(keyFile== NULL || cipherFile==NULL){
		printk("At least one of the files (key/cipher) is null\n");
		return -EINVAL;
	}

	while(i<length){
		off = modLong((long)i,(long)keySize);
		oldfs = get_fs();
		set_fs(get_ds());
		retVal = vfs_llseek(keyFile, off, SEEK_SET);
		set_fs(oldfs);
		if(retVal!=off){
			printk(KERN_ALERT "ERROR: can't seek key file. Returned %d\n",retVal);
			break;
		}

		oldfs = get_fs();
		set_fs(get_ds());
		retVal = vfs_read(keyFile, keyBuffer, 1, &(keyFile->f_pos));
		set_fs(oldfs);
		if(retVal!=1){
			printk(KERN_ALERT "ERROR: can't read key file. Returned %d\n",retVal);
			break;
		}

		if(buffer[i] == '\0'){
			oldfs = get_fs();
			set_fs(get_ds());
			retVal = vfs_write(cipherFile, '\0', numToWrite, &(cipherFile->f_pos));
			set_fs(oldfs);
			if(retVal!=1){
				printk(KERN_ALERT "ERROR: can't write cipher. Returned %d\n",retVal);
				break;
			}
			break;
		}

		put_user(buffer[i] ^ keyBuffer[0], &(cipherBuffer[0]));
		printk("In write, i wrote %c which is %c and %c to pos %d\n"
			,buffer[i] ^ keyBuffer[0],buffer[i],keyBuffer[0],cipherFile->f_pos);
		oldfs = get_fs();
		set_fs(get_ds());
		retVal = vfs_write(cipherFile, buffer[i] ^ keyBuffer[0], numToWrite, &(cipherFile->f_pos));
		set_fs(oldfs);
		if(retVal!=1){
			printk(KERN_ALERT "ERROR: can't write cipher. Returned %d\n",retVal);
			break;
		}
		i++;
		num_of_bytes_written++;
	}
	cipherBuffer[0]='/0';
	oldfs = get_fs();
		set_fs(get_ds());
		retVal = vfs_write(cipherFile, (__force char __user *)cipherBuffer, numToWrite, &(cipherFile->f_pos));
		set_fs(oldfs);

	return num_of_bytes_written;
}


long set_devices(struct file* filp,unsigned int cmd, unsigned long arg){
	struct file *keyFile;
	struct file *cipherFile;
	struct kstat ks;
	int error;
	mm_segment_t oldfs;
	char* path=(char*) arg;
	int i=0;

	if(filp->private_data==NULL){
		printk("private data is null\n");
		return -1;
	}
	if(cmd == 1){//cipher

		((pData*)(filp->private_data))->cipherFileName=(char*)kmalloc(100,GFP_KERNEL);
		while(*(path+i)!='\0'){
			if(get_user(((char*)((pData*)(filp->private_data))->cipherFileName)[i],path+i)!=0){
				printk("Error in get user of cipher file\n");
				return -1;
			}
			i++;
		}
		printk("arg is %s\n",path);
		oldfs = get_fs();
		set_fs(get_ds());
		((pData*)(filp->private_data))->cipherFile = filp_open(((pData*)(filp->private_data))->cipherFileName,O_RDWR | O_APPEND,0777);
		set_fs(oldfs);
		cipherFile=((pData*)(filp->private_data))->cipherFile;

		

		if(IS_ERR(cipherFile)){
			printk("Error opening cipher file, error code is %ld\n",PTR_ERR(cipherFile));
			return -1;
		}
		oldfs = get_fs();
		set_fs(get_ds());
		error=vfs_stat(path,&ks);
		set_fs(oldfs);
		if(error){
			printk("error using stat of cipher\n");
			return -1;
		}
		else{
			((pData*)(filp->private_data))->cipherSize=ks.size;
		}
	}
	if(cmd == 3){//key
		((pData*)(filp->private_data))->keyFileName=(char*)kmalloc(100,GFP_KERNEL);
		while(*(path+i)!='\0'){
			if(get_user(((char*)((pData*)(filp->private_data))->keyFileName)[i],path+i)!=0){
				printk("Error in get user of cipher file\n");
				return -1;
			}
			i++;
		}

		oldfs = get_fs();
		set_fs(get_ds());
		((pData*)(filp->private_data))->keyFile = filp_open(((pData*)(filp->private_data))->keyFileName,O_RDONLY,0777);
		set_fs(oldfs);
		keyFile=((pData*)(filp->private_data))->keyFile;
		if(IS_ERR(keyFile)){
			printk("Error opening key file, error code is %ld\n",PTR_ERR(keyFile));
			return -1;
		}
		oldfs = get_fs();
		set_fs(get_ds());
		error=vfs_stat(path,&ks);
		set_fs(oldfs);
		if(error){
			printk("error using stat of key\n");
			return -1;
		}
		else{
			((pData*)(filp->private_data))->keySize=ks.size;
			printk("size of key is %d\n",ks.size);
		}

	}
	return 0;
}



static loff_t device_llseek(struct file* filp,loff_t offset,int position){
	int ret=0;
	mm_segment_t oldfs;
	loff_t offKey;
	loff_t keySize=((pData*)(filp->private_data))->keySize;
	if(position==SEEK_SET){
		filp->f_pos=offset;
		offKey=modLong((long)offset,keySize);
		((pData*)(filp->private_data))->keyFile->f_pos=offKey;
	}
	else if(position==SEEK_CUR){
		filp->f_pos+=offset;
		offKey=modLong((long)(offset+((pData*)(filp->private_data))->keyFile->f_pos),keySize);
		((pData*)(filp->private_data))->keyFile->f_pos=offKey;
	}
	else{
		filp->f_pos=((pData*)(filp->private_data))->cipherSize+offset;
		offKey=modLong((long)(offset-1),(long)keySize);
		((pData*)(filp->private_data))->keyFile->f_pos=offKey;
	}
	oldfs =get_fs();
	set_fs(get_ds());
	ret = vfs_llseek(((pData*)(filp->private_data))->cipherFile,offset,position);
	set_fs(oldfs);
	if(ret<0){
		printk("Error in llseek of cipherfile\n");
		return -1;
	}
	printk("Seek to %ld\n",offset);
	return offset;
}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */
struct file_operations Fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
	.unlocked_ioctl= set_devices,
	.llseek = device_llseek,
};

/* Called when module is loaded. 
 * Initialize the module - Register the character device */
static int __init simple_init(void)
{
    /* init dev struct*/
	memset(&device_info, 0, sizeof(struct chardev_info));
	spin_lock_init(&device_info.lock);    

    /* Register a character device. Get newly assigned Major num */
    Major = register_chrdev(0, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

    /* 
     * Negative values signify an error 
     */
     if (Major < 0) {
     	printk(KERN_ALERT "%s failed with %d\n",
     		"Sorry, registering the character device ", Major);
     	return Major;
     }

     printk("Registeration is a success. The Major device number is %d.\n", Major);
     printk("If you want to talk to the device driver,\n");
     printk("you have to create a device file:\n");
     printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, Major);
     printk("You can echo/cat to/from the device file.\n");
     printk("Dont forget to rm the device file and rmmod when you're done\n");

     return 0;
 }

/* Cleanup - unregister the appropriate file from /proc */
 static void __exit simple_cleanup(void)
 {
    /* 
     * Unregister the device 
     * should always succeed (didnt used to in older kernel versions)
     */
     unregister_chrdev(Major, DEVICE_RANGE_NAME);
 }

 module_init(simple_init);
 module_exit(simple_cleanup);


/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

/*
* Modified - @author Shrikant Nimhan 
*
* Attributes -  https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
*               https://stackoverflow.com/questions/59000547/clear-buffer-user-data-before-doing-another-write-on-a-linux-device-driver
*/


#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include "aesd-circular-buffer.h"
#include "aesdchar.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("shrikant"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *aesddevice;

	PDEBUG("open");
	/**
	 * TODO: handle open
	 */

	aesddevice = container_of(inode->i_cdev,struct aesd_dev,cdev);

	filp->private_data = aesddevice;

	return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
	PDEBUG("release");
	/**
	 * TODO: handle release
	 */
	return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	struct aesd_dev *aesddevice;
	struct aesd_buffer_entry *entry;
	const char* pointer_to_buffer;
	ssize_t retval = 0;
	size_t offset = 0;
	size_t bytes_to_read;
	int check;
    
	aesddevice = filp->private_data;

	entry = aesd_circular_buffer_find_entry_offset_for_fpos(&aesddevice->circularbuffer,*f_pos,&offset);

	PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
	/**
	 * TODO: handle read
	 */


	check = mutex_lock_interruptible(&aesddevice->mutex_lock);
	if (check != 0){

		return check;

	}

	if (entry != NULL){

		bytes_to_read = count + offset;

		if( bytes_to_read <= entry->size){

			retval = count;

			pointer_to_buffer = entry->buffptr + offset;

			check = copy_to_user(buf,pointer_to_buffer,retval);
			if (check != 0){
				
				return -EFAULT;
				
			}

			*f_pos += retval;

			mutex_unlock(&aesddevice->mutex_lock);

		}

		else {

			retval = entry->size - offset;

			pointer_to_buffer = entry->buffptr + offset;

			check = copy_to_user(buf,pointer_to_buffer,retval);
			if (check != 0){
				
				return -EFAULT;
				
			}

			*f_pos += retval;

			mutex_unlock(&aesddevice->mutex_lock);

		}

	}

	else {

		    mutex_unlock(&aesddevice->mutex_lock);

			return 0;


	}

	return retval;

}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{

    struct aesd_dev *aesddevice;

	const char* pointer_to_replaced = NULL;
	char* pointer_to_newline = NULL;

	ssize_t retval;
	ssize_t uncopied_bytes;

	int check;

	size_t untransferred_bytes;

	aesddevice = filp->private_data;

	/**
	 * TODO: handle write
	 */
	check  = mutex_lock_interruptible(&aesddevice->mutex_lock);
    
	if (check != 0){

		return check;

	}
	

	if (aesddevice->bufferentry.size == 0) 
	{
		
		aesddevice->bufferentry.buffptr = kmalloc(count*sizeof(char), GFP_KERNEL);  

		if ( !aesddevice->bufferentry.buffptr)  
		{
		
			mutex_unlock(&aesddevice->mutex_lock);
			retval = -ENOMEM; 
	        return retval;

		}
	}
	else 
	{
	       aesddevice->bufferentry.buffptr = krealloc(aesddevice->bufferentry.buffptr, aesddevice->bufferentry.size + count, GFP_KERNEL); 
		
		if ( !aesddevice->bufferentry.buffptr) 
		{
			mutex_unlock(&aesddevice->mutex_lock);
	        return retval;

		}
	 }

	
   
    uncopied_bytes=copy_from_user((void *)(&aesddevice->bufferentry.buffptr[aesddevice->bufferentry.size]), buf, count);
	
	untransferred_bytes = count - uncopied_bytes;
	
	aesddevice->bufferentry.size += untransferred_bytes;

	pointer_to_newline = (char *)memchr(aesddevice->bufferentry.buffptr, '\n', aesddevice->bufferentry.size);
	
	if (pointer_to_newline != NULL) 
	{
		
	   pointer_to_replaced = aesd_circular_buffer_add_entry(&aesddevice->circularbuffer, &aesddevice->bufferentry);
	   
	   if (pointer_to_replaced != NULL) 
        {

			kfree(pointer_to_replaced);
		
		}
	   
	  aesddevice->bufferentry.size = 0;
      aesddevice->bufferentry.buffptr = 0;
	}

  
     *f_pos = 0;
     
     mutex_unlock(&aesddevice->mutex_lock);

	 
     return untransferred_bytes;

}


struct file_operations aesd_fops = {
	.owner =    THIS_MODULE,
	.read =     aesd_read,
	.write =    aesd_write,
	.open =     aesd_open,
	.release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
	int err, devno = MKDEV(aesd_major, aesd_minor);

	cdev_init(&dev->cdev, &aesd_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &aesd_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err) {
		printk(KERN_ERR "Error %d adding aesd cdev", err);
	}
	return err;
}

int aesd_init_module(void)
{
	dev_t dev = 0;
	int result;
	result = alloc_chrdev_region(&dev, aesd_minor, 1,
			"aesdchar");

	aesd_major = MAJOR(dev);

	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", aesd_major);
		return result;
	}

	
	memset(&aesd_device,0,sizeof(struct aesd_dev));

	/**
	 * TODO: initialize the AESD specific portion of the device
	 */


	mutex_init(&aesd_device.mutex_lock);	

	result = aesd_setup_cdev(&aesd_device);

	if( result ) {
		unregister_chrdev_region(dev, 1);
	}

	return result;

}

void aesd_cleanup_module(void)
{
	dev_t devno = MKDEV(aesd_major, aesd_minor);

	cdev_del(&aesd_device.cdev);

	/**
	 * TODO: cleanup AESD specific poritions here as necessary
	 */
	clean_aesd_buffer(&aesd_device.circularbuffer);

	unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
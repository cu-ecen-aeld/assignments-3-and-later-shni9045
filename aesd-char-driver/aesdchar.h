/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */

// Attributes - https://stackoverflow.com/questions/59000547/clear-buffer-user-data-before-doing-another-write-on-a-linux-device-driver

#ifndef AESD_CHAR_DRIVER_AESDCHAR_H_
#define AESD_CHAR_DRIVER_AESDCHAR_H_

#define AESD_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef AESD_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "aesdchar: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif


struct aesd_dev
/*
 * aesdchar.h
 *
 *  Created on: Oct 23, 2019
 *      Author: Dan Walkes
 */{
	/**
	 * TODO: Add structure(s) and locks needed to complete assignment requirements
	 */
	struct cdev cdev;	  /* Char device structure		*/
	struct aesd_circular_buffer circularbuffer;
	struct aesd_buffer_entry bufferentry;
	struct mutex mutex_lock;
	
};


#endif /* AESD_CHAR_DRIVER_AESDCHAR_H_ */
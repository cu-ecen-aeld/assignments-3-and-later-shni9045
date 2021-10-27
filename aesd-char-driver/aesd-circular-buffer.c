/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

/*
* Modified - @author Shrikant Nimhan 
*
* Attributes -  https://embeddedartistry.com/blog/2017/05/17/creating-a-circular-buffer-in-c-and-c/
*/

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */

   struct aesd_buffer_entry* entry_ptr;                // Pointer to iterate over buffer entries
   uint8_t index;                                      // Index into buffer entry

   size_t temp_size=0;                                 // Variable to track entry sizes
   size_t hold_size=0;                                 // Variable to hold entry sizes when offset is possible & found

   int offset_possible = 0;                            // Variable to track if offset is possible

   uint8_t helper_index=0;                              // Variable to limit iteration over buffer

   // Logic if offset with first byte is asked
   if (char_offset == 0 ){

       *entry_offset_byte_rtn = 0;

       return  &(buffer->entry[buffer->out_offs]);

   }

   index  = buffer->out_offs;


   // Iterate with wrapping around index upon update
   for(entry_ptr=&((buffer)->entry[index]); \
			helper_index<AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED ; \
			index = ((index+1)%AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED), entry_ptr=&((buffer)->entry[index])){

       
       temp_size += entry_ptr->size;

       helper_index++;

       if(temp_size  >= (char_offset +1)){

           offset_possible = 1;                                // offset is possible
               
           hold_size = temp_size -entry_ptr->size;             // reduce extra iterated entry size

           break;

       }

   }

   // If offset has been found
   if(offset_possible){
       
       size_t char_pos;
           
       char_pos = char_offset - hold_size;                   // Calculate position of byte into entry
       *entry_offset_byte_rtn = char_pos;
           

       return entry_ptr;                                    // return entry

   }

    return NULL;

}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
const char* aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
   const char* temp_entry = NULL;


	if(buffer->full == true){
		
		temp_entry = buffer->entry[buffer->out_offs].buffptr;

	}

   // Store new entry at current write location in structure
   buffer->entry[buffer->in_offs] = *add_entry;
   
   // Check if buffer is full and advance read location
   if(buffer->full == true){

       buffer->out_offs = ((buffer->out_offs + 1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));

   }
   
   // Advance write location
   buffer->in_offs = ((buffer->in_offs + 1) % (AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED));
   
   // Check and set buffer full location
   if(buffer->out_offs == buffer->in_offs ) {

       buffer->full = true;

   }

   return temp_entry;

}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    
    memset(buffer,0,sizeof(struct aesd_circular_buffer));


}

void clean_aesd_buffer(struct aesd_circular_buffer* buffer){

    struct aesd_buffer_entry *entry;
	uint8_t index;


	AESD_CIRCULAR_BUFFER_FOREACH(entry,buffer,index) 
	{

		if (entry->buffptr != NULL)
		{

#ifndef __KERNEL__
		
		free((void*)entry->buffptr);	
#else
		kfree(entry->buffptr);
		
		
#endif
         }

	}
	

}
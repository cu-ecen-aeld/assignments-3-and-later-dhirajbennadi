/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

static int write = 0;
static int read = 0;
static int length = 0;

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
    size_t totalLengthofChars = 0;
    struct aesd_buffer_entry *retVal = NULL;
    for (int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
    {
        totalLengthofChars += buffer->entry[i].size;
    }

    size_t temp = char_offset;
    int i = 0;
    int counter = 0;

    if (char_offset < totalLengthofChars)
    {
        for (i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++)
        {
            temp -= buffer->entry[i].size;
            counter += buffer->entry[i].size;
            if (temp <= 0)
            {
                break;
            }
        }

        retVal = buffer->entry[i].buffptr;
        *entry_offset_byte_rtn = temp * -1;
    }
    else
    {
        retVal = NULL;
    }

    return retVal;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */

   if(isFull())
   {
    buffer->entry[write].buffptr = add_entry->buffptr;
    buffer->entry[write].size = add_entry->size;
    write = write + 1 % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    read = read + 1 % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
   }
   else
   {
    buffer->entry[write].buffptr = add_entry->buffptr;
    buffer->entry[write].size = add_entry->size;
    write = write + 1 % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
   }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}


bool isFull()
{
    if(length == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
    {
        return true;
    }

    return false;
}
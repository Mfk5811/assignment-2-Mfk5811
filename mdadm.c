#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "mdadm.h"
#include "jbod.h"


static int mounted = 0;

int mdadm_mount(void) {
  
  // check if the system needs to be mouted
  if(mounted){
    return -1;
  }

  if (jbod_operation(JBOD_MOUNT << 12, NULL) == 0) {
     mounted = 1;  
     return 1;
  }


return -1;
}


// Does the opposite of mdadm_mount
int mdadm_unmount(void) {
  if (!mounted) {
    return -1;  // System is already unmounted
  }

  if (jbod_operation(JBOD_UNMOUNT << 12, NULL) == 0) {
      mounted = 0;  // Successfully unmounted, set mounted to 0
      return 1;     // Unmount succeeded
  }
  return -1;  // Unmount failed
}




int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf)  {
  

 if(!mounted){
    return -3;   // When system isnt mounted Based on README file
  }

  if (read_len == 0){
    return 0;
  }

  if (read_len > 1024) {
     return -2;  // returns 2 based on README file
  }

  if (read_buf == NULL){
    return -4;
  }

  uint32_t total_capacity = JBOD_NUM_DISKS * JBOD_NUM_BLOCKS_PER_DISK * JBOD_BLOCK_SIZE;
  if (start_addr + read_len > total_capacity) {
      return -1;  // return -1 when Address is out of bounds based README file
  }

  uint8_t buffer[JBOD_BLOCK_SIZE];  
  uint32_t bytes_read = 0;

   
    while (bytes_read < read_len) {
       
        uint32_t block_addr = (start_addr + bytes_read) / JBOD_BLOCK_SIZE;
        uint8_t disk_id = block_addr / JBOD_NUM_BLOCKS_PER_DISK;
        uint8_t block_id = block_addr % JBOD_NUM_BLOCKS_PER_DISK;

        uint32_t seek_disk_op = (disk_id << 0) | (JBOD_SEEK_TO_DISK << 12);
        if (jbod_operation(seek_disk_op, NULL) != 0) {
            return -4;  
        }

      
        uint32_t seek_block_op = (block_id << 4) | (JBOD_SEEK_TO_BLOCK << 12);
        if (jbod_operation(seek_block_op, NULL) != 0) {
            return -4; 
        }

      
        uint32_t read_op = (JBOD_READ_BLOCK << 12);
        if (jbod_operation(read_op, buffer) != 0) {
            return -4;  
        }

       
        uint32_t offset_in_block = (bytes_read + start_addr )% JBOD_BLOCK_SIZE;
        uint32_t bytes_to_copy = JBOD_BLOCK_SIZE - offset_in_block;
        if (bytes_read + bytes_to_copy > read_len) {
            bytes_to_copy = read_len - bytes_read;
        }
        memcpy(read_buf + bytes_read, buffer + offset_in_block, bytes_to_copy);

        bytes_read += bytes_to_copy;
    }

    return bytes_read;

}



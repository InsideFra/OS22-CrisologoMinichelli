#ifndef _SWAPFILE_H_
#define _SWAPFILE_H_

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>

#define SWAPSPACE       9*1024*1024
#define SWAPFILE_SIZE   SWAPSPACE/4096  //max number of pages that can be swapped on disk

struct list_param{
    uint16_t p_number;
    uint16_t p_pid;
    uint16_t p_offset;
};

int swapfile_init(void);
int swapfile_check(uint8_t page_num, pid_t pid);

int sf_pageSearch(uint8_t page_num, pid_t pid);
int sf_listSearch(uint8_t page_num, pid_t pid);
int sf_freeSearch(void);

int swapIn(int index, uint32_t RAM_address);
int swapOut(uint32_t RAM_address);

#endif /* _VM_H_ */

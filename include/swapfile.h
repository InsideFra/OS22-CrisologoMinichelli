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
    uint32_t p_number;
    pid_t p_pid;
    uint32_t p_offset;
    bool Dirty;

};

int swapfile_init(void);
int swapfile_check(uint32_t page_num, pid_t pid);
int swapfile_checkv1(uint32_t page_num, pid_t pid);

int sf_pageSearch(uint32_t page_num, pid_t pid);
int sf_listSearch(uint32_t page_num, pid_t pid);
int sf_freeSearch(void);

int swapIn(int index);
int swapOut(uint32_t* RAM_address);

#endif /* _VM_H_ */

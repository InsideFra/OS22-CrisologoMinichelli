#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>

#define SWAPSPACE       9*1024*1024
#define SWAPFILE_SIZE   SWAPSPACE/4096  //max number of pages that can be swapped on disk

char filename[] = "swapfile";

struct list_param{
    uint8_t p_number;
    pid_t p_pid;
    off_t p_offset;
};

int swapfile_init(void);
int swapfile_check(uint8_t page_num, pid_t pid);
int sf_listSearch(uint8_t page_num, pid_t pid);
int swapIn(off_t p_offset, uint32_t *address);
int swapOut(uint32_t RAM_address);
#ifndef _PAGE_H_
#define _PAGE_H_

#include <types.h>
#include <vm.h>

// Page-table lenght register
//The size in kB of a page table entry
#define PTLR sizeof(struct RAM_PG_)

// Page-table base register
// The start address in memory where to store page table (stack like)
//#define PTBR_ 1

// The size of the allocaed space to store page table

unsigned int PAGETABLE_ENTRY;

#define Process_PG_ENTRY 256

#define MAX_CODE_SEGMENT_PAGES  (uint32_t)2
#define MAX_DATA_SEGMENT_PAGES  (uint32_t)4
#define MAX_HEAP_SEGMENT_PAGES  (uint32_t)4
#define MAX_STACK_SEGMENT_PAGES (uint32_t)4

#define mainPG(i) ((struct PG_)*(main_PG+i*(sizeof struct PG_)))

// inverted page table
struct RAM_PG_ {
    _Bool CachingDisabled;
    _Bool Referenced;
    _Bool Modified;
    _Bool Valid;
    uint32_t clock;
    uint32_t pid;
    uint32_t page_number;
};

// struct PG_ {
//     _Bool Valid;
//     uint32_t frame_number;
// };

// struct DISK_PG_ {
//     _Bool Valid;
//     uint32_t frame_number;
//     uint32_t page_number;
// };

// struct process_PG {
//     _Bool CachingDisabled;
//     _Bool Referenced;
//     _Bool Modified;
//     _Bool Valid;
//     uint8_t pagenumber;
//     uint8_t Protection;
//     uint32_t frame_number;
// };


// this struct should be at the top of process_PG
struct PG_Info {
    vaddr_t  code_vaddr;
    uint8_t  code_entries;
    vaddr_t  data_vaddr;
    uint8_t  data_entries;
    uint8_t  heap_entries;
    uint8_t  stack_entries;
};

#endif /* _PAGE_H_ */


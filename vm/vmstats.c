/* code for tracking stats */
#include <vm.h>
#include <lib.h>
#include <kern/time.h>
#include <clock.h>

/* The number of TLB misses that have occurred (not including faults that cause a program to crash) */
unsigned int TLB_Faults = 0; 

/* The number of TLB misses for which there was free space in the TLB to add the new TLB entry 
* (i.e., no replacement is required)
*/
unsigned int TLB_Faults_wFree = 0;

/* The number of TLB misses for which there was no free space for the new TLB entry, so replacement was required. */
unsigned int TLB_Faults_wReplace = 0;

/* The number of times the TLB was invalidated 
* (this counts the number times the entire TLB is invalidated NOT the number of TLB entries invalidated) 
*/
unsigned int TLB_Invalidations = 0;

/* The number of TLB misses for pages that were already in memory */
unsigned int TLB_Reloads = 0;

/* The number of TLB misses that required a new page to be zero-filled. */
unsigned int PF_Zeroed = 0;

/* The number of TLB misses that required a page to be loaded from disk. */
unsigned int PF_Disk = 0;

/* The number of page faults that require getting a page from the ELF file */
unsigned int PF_ELF = 0;

/* he number of page faults that require getting a page from the swap file. */
unsigned int PF_Swapfile = 0;

/* The number of page faults that require writing a page to the swap file. */
unsigned int SF_Writes = 0;

extern struct timespec duration_pageSearch;
extern struct timespec duration_VMFAULTREAD1, duration_VMFAULTREAD2, duration_swap;

void print_vm_stat(void) {
    kprintf( "sys161: (a) TLB Faults: %d\n", TLB_Faults);
    kprintf( "sys161: (b) TLB misses with    free space in the TLB: %d\n", TLB_Faults_wFree);
    kprintf( "sys161: (c) TLB misses without free space in the TLB: %d\n", TLB_Faults_wReplace);
    kprintf( "sys161: (d) TLB misses with pages in memory: %d\n", TLB_Reloads);
    kprintf( "sys161: (e) TLB misses that required a page to be loaded from disk: %d\n", PF_Disk);
    kprintf( "sys161: (f) TLB misses that required a page to be loaded from the ELF  file: %d\n", PF_ELF);
    kprintf( "sys161: (g) TLB misses that required a page to be loaded from the swap file: %d\n", PF_Swapfile);
    kprintf( "sys161: (h) TLB misses that required a page to be loaded to   the swap file: %d\n", SF_Writes);
    // kprintf( "sys161: (i) Time passed in pageSearch : %llu.%09lu seconds\n", 
    //             	(unsigned long long) duration_pageSearch.tv_sec,
	//                 (unsigned long) duration_pageSearch.tv_nsec);
    // kprintf( "sys161: (i) Time passed in looked function1: %llu.%09lu seconds\n", 
    //             	(unsigned long long) duration_VMFAULTREAD1.tv_sec,
	//                 (unsigned long) duration_VMFAULTREAD1.tv_nsec);
    // kprintf( "sys161: (i) Time passed in looked function2: %llu.%09lu seconds\n", 
    //             	(unsigned long long) duration_VMFAULTREAD2.tv_sec,
	//                 (unsigned long) duration_VMFAULTREAD2.tv_nsec);
    kprintf( "sys161: (i) Time passed in swapIN + swapOUT: %llu.%09lu seconds\n", 
                	(unsigned long long) duration_swap.tv_sec,
	                (unsigned long) duration_swap.tv_nsec);
}
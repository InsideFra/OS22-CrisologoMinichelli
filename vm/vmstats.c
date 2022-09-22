/* code for tracking stats */

void print_vm_stat(void);

/* The number of TLB misses that have occurred (not including faults that cause a program to crash) */
unsigned int TLB_Faults = 0; 

/* The umber of TLB misses for which there was free space in the TLB to add the new TLB entry 
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

void print_vm_stat(void) {
    
}
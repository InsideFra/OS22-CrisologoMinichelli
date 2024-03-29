/* code
for managing and manipulating the swapfile */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <synch.h>
#include <spl.h>
#include <pt.h>
#include <swapfile.h>
#include <machine/tlb.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <vm_tlb.h>
#include <current.h>
#include <kern/time.h>
#include <clock.h>

struct timespec duration_swap;
struct timespec before, after, duration;

struct list_param sf_list[SWAPFILE_SIZE];   
struct lock *sf_lock;

extern struct invertedPT *main_PG;

char filename[] = "swapfile";

int swapfile_init(void){
	struct vnode *swapfile;
    int result;

	result = vfs_open(filename, O_CREAT | O_RDWR, 0, &swapfile);
    KASSERT(swapfile != NULL);

    if(result){
       panic("Something has gone wrong with swapfile creation\n"); 
    } else {
       kprintf("VM: swapfile opened correctly\n"); 
    }

    //swapfile has to be empty
    for(int i = 0; i<SWAPFILE_SIZE; i++){
       sf_list[i].p_number = 0;
       sf_list[i].p_pid = 0;
       sf_list[i].Dirty = 0;
       sf_list[i].p_offset = i;
    }

    //swapfile_lock creation to protect swapfile_list

    //sf_lock = lock_create("swap");

    return 0;
}

/**
 * Find the index in the swap file list using page number and pid;
 * @param {uint32_t} page_num - Index number in the swap file.
 * @param {pid_t} pid - Process ID (not supported yet).
 * @return {int} index if found in the list or noEntryFound.
 */
int swapfile_checkv1(uint32_t page_num, pid_t pid){
    //we have to check if the requested page is saved into swapfile
    //off_t page_offset;
    int index;
    if((index = sf_pageSearch(page_num, pid)) < 0){
	
        /*--------------------PAGE NOT FOUND--------------------*/

        return noEntryFound;
    } else if(index >= 0){

        /*--------------------PAGE FOUND--------------------*/
		return index;
	}
    return noEntryFound;
}

int sf_pageSearch(uint32_t page_num, pid_t pid){
    for(uint32_t i = 0; i < SWAPFILE_SIZE; i++){
        if((page_num == sf_list[i].p_number) && (pid == sf_list[i].p_pid)){
            //page find in the swap list
            return i;
        } else {
            continue;
        }
    }
    //page not found
    return noEntryFound;
}

int sf_freeSearch(void){
    for(int i=0; i<SWAPFILE_SIZE; i++){
        if((sf_list[i].p_number == 0) && (sf_list[i].p_pid == 0)){
            //free slot found
            return i; 
        }

    }
    return noEntryFound;
}

/**
 * Find the index using swapfile_checkv1(p_num, curproc->pid);
 * @param {int} index - Index number in the swap file.
 * @return {int} 0 if no error.
 */
int swapIn(int index){
    gettime(&before);
	int RAM_address;
	int result;
    int err;
    off_t offset;

	struct vnode *v;
    struct iovec iov;
    struct uio sfuio;

    /*---------------------------------- PT LIST and TLB LIST UPDATE ------------------------------------*/
	if(!(RAM_address = alloc_kpages(1))){
        panic("swapIn not allowed!\n");
	}

    unsigned int pt_index = ((uint32_t)RAM_address - MIPS_KSEG0)/PAGE_SIZE; 
    unsigned int vaddress = sf_list[index].p_number * PAGE_SIZE;
    pid_t pid = sf_list[index].p_pid;
    bool Dirty = sf_list[index].Dirty;
    
    if(addPT(pt_index, vaddress, pid, Dirty)){
    	return EINVAL;
    }

    if(addTLB(vaddress, pid)){
    	return EINVAL;
    }

    offset = sf_list[index].p_offset*PAGE_SIZE; 

	if((result = vfs_open(filename, O_RDWR, 0, &v))>0){
		panic("ERROR in SWAPFILE opening operation\n"); 
	}

    uio_kinit(&iov, &sfuio, (uint32_t *)RAM_address, PAGE_SIZE, offset, UIO_READ);
	err = VOP_READ(v, &sfuio);
	if (err) {
		kprintf("%s: Read error: %s\n", filename, strerror(err));
		vfs_close(v);
		return -1;
	}
    //page copied
    vfs_close(v);

    /*---------------------------------- SF LIST UPDATE -------------------------------------------------*/
    sf_list[index].p_number = 0; 
    sf_list[index].p_pid = 0;
    sf_list[index].Dirty = 0; 
    //kprintf("swapIN: Index: %d, pAddr: 0x%x\n", index, (uint32_t)RAM_address);
    gettime(&after);
    timespec_sub(&after, &before, &duration);
    timespec_add(&duration, &duration_swap, &duration_swap);
    return 0;
}

/**
 * Used to SWAPOUT to disk a frame.
 * @param {uint32_t} RAM_address - The first physical address of a frame.
 * @return {int} 0 if everything ok.
 */
int swapOut(uint32_t* RAM_address){
    gettime(&before);
	int result, err;
    off_t offset;
	struct vnode *v;
    struct iovec iov;
    struct uio sfuio;

    // Is there almost one slot free in swapfile list??
    int index = sf_freeSearch();
    if(index == noEntryFound){
        //no space on disk
        panic("ERROR: Out of swap space!!\n");
        panic("ERROR: disk is full!!\n");
    }
    offset = sf_list[index].p_offset*PAGE_SIZE;

	if((result = vfs_open(filename, O_RDWR, 0, &v))>0){
		panic("ERROR in SWAPFILE opening operation\n"); 
    }

    uio_kinit(&iov, &sfuio, RAM_address, PAGE_SIZE, offset, UIO_WRITE);
	err = VOP_WRITE(v, &sfuio);
	if (err) {
		kprintf("%s: Write error: %s\n", filename, strerror(err));
		vfs_close(v);
		return -1;
	}
    vfs_close(v);
    // page swapped
    // list updating: TLB_table, PAGE_table, sf_list

    /*---------------------------------- SF LIST UPDATE -------------------------------------------------*/
    unsigned int pt_index = ((uint32_t)RAM_address - MIPS_KSEG0)/PAGE_SIZE;  
    //kprintf("(swapOUT  ): [%3d] PN: %x\n", index, main_PG[pt_index].page_number);
    sf_list[index].p_number = main_PG[pt_index].page_number; 
    sf_list[index].p_pid = main_PG[pt_index].pid;
    sf_list[index].Dirty = main_PG[pt_index].Dirty;

    /*---------------------------------- PT LIST and TLB LIST UPDATE ------------------------------------*/
    free_kpages((uint32_t)RAM_address);
    //kprintf("swapOut: pAddr: 0x%x\n", RAM_address);

    gettime(&after);
    timespec_sub(&after, &before, &duration);
    timespec_add(&duration, &duration_swap, &duration_swap);

    return 0;
}
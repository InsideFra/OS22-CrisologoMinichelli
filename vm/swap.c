#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <synch.h>
#include <spl.h>
#include <page.h>
#include <swap.h>
#include <machine/tlb.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>

struct list_param sf_list[SWAPFILE_SIZE];   
struct lock *sf_lock;
extern struct RAM_PG_ *main_PG;

int addPT(uint32_t frame_index, vaddr_t vaddr, uint32_t pid);

int swapfile_init(void){
	struct vnode *swapfile;
    int result;

	result = vfs_open(filename, O_CREAT | O_RDWR, 0, &swapfile);
    KASSERT(swapfile != NULL);
    
    if(result){
       kprintf("Something has gone wrong with swapfile creation\n"); 
    } else {
       kprintf("swapfile opened correctly\n"); 
    }

    //swapfile has to be empty
    for(int i = 0; i<SWAPFILE_SIZE; i++){
       sf_list[i].p_number = 0;
       sf_list[i].p_pid = 0;
       sf_list[i].p_offset = i;
    }

    //swapfile_lock creation to protect swapfile_list

    sf_lock = lock_create("swap");
        
}

int swapfile_check(uint8_t page_num, pid_t pid){
    //we have to check if the requested page is saved into swapfile
    off_t page_offset;
    int index;
    uint32_t address;
    uint32_t victim_page;
    if((index = sf_pageSearch(page_num, pid)) < 0){

        /*--------------------PAGE NOT FOUND--------------------*/
        
        return 1;
    } else if(index >= 0){

        /*--------------------PAGE FOUND------------------------*/

        //before swap-in operation, we need to check if there is space in RAM
            //address can be:
            // 0 => no space in RAM, swapOut operation is needed
            // >0 => slot available in RAM, "address" is the address in RAM where we can store swapped page from DISK 
        if(!(address = alloc_kpages(1, 0))){
            victim_page = victim_pageSearch();
            address = victim_page*PAGE_SIZE + MIPS_KSEG0;
            swapOut(address);
        }
        if(swapIn(index, address)){
            //error in swapping operation in RAM
            return -1;
        } else {
            //page copied in RAM
            return 0;
        }
        
    }
}


int sf_pageSearch(uint8_t page_num, pid_t pid){
    uint8_t i = 0;
    
    for(i = 0; i<SWAPFILE_SIZE; i++){
        if((page_num == sf_list->p_number) && (pid == sf_list->p_pid)){
            //page find in the swap list
            return i;
        } else {
            continue;
        }
    }
    //page not found
    return noEntryFound;
}

int sf_freeSearch(){
    for(int i=0; i<SWAPFILE_SIZE; i++){
        if((sf_list[i].p_number == 0) && (sf_list[i].p_pid == 0)){
            //free slot found
            return i; 
        }

    }
    return noEntryFound;
}


int swapIn(int index, uint32_t RAM_address){
	int result;
    int err;
    off_t offset;
	
	struct vnode *v;
    struct iovec iov;
    struct uio sfuio;

    offset = sf_list[index].p_offset*PAGE_SIZE; 

	if((result = vfs_open(filename, O_RDWR, 0, &v))>0){
		panic("ERROR in SWAPFILE opening operation\n"); 
	}

    uio_kinit(&iov, &sfuio, &RAM_address, PAGE_SIZE, offset, UIO_READ);
	err = VOP_READ(v, &sfuio);
	if (err) {
		kprintf("%s: Read error: %s\n", filename, strerror(err));
		vfs_close(v);
		return -1;
	}
    //page copied
    vfs_close(v);
    return 0;

    /*---------------------------------- PT LIST and TLB LIST UPDATE ------------------------------------*/
    unsigned int pt_index = (RAM_address - MIPS_KSEG0)/PAGE_SIZE; 
    unsigned int vaddress = sf_list[index].p_number * PAGE_SIZE;
    pid_t pid = sf_list[index].p_pid;
    addPT(pt_index, vaddress, pid);

    /*---------------------------------- SF LIST UPDATE -------------------------------------------------*/
    sf_list[index].p_number = 0; 
    sf_list[index].p_pid = 0; 
}

int swapOut(uint32_t RAM_address){
	int result, err;
    off_t offset;
	struct vnode *v;
    struct iovec iov;
    struct uio sfuio;

    // Is there almost one slot free in swapfile list??
    int index = sf_freeSearch();
    if(index == noEntryFound){
        //no space on disk
        panic("ERROR: disk is full!!\n");
    }
    offset = sf_list[index].p_offset*PAGE_SIZE;

	if((result = vfs_open(filename, O_RDWR, 0, &v))>0){
		panic("ERROR in SWAPFILE opening operation\n"); 
    }

    uio_kinit(&iov, &sfuio, &RAM_address, PAGE_SIZE, offset, UIO_WRITE);
	err = VOP_WRITE(v, &sfuio);
	if (err) {
		kprintf("%s: Write error: %s\n", filename, strerror(err));
		vfs_close(v);
		return -1;
	}
    vfs_close(v);
    // page swapped
    // list updating: TLB_table, PAGE_table, sf_list

    /*---------------------------------- PT LIST and TLB LIST UPDATE ------------------------------------*/
    unsigned int pt_index = (RAM_address - MIPS_KSEG0)/PAGE_SIZE; 
    unsigned int vaddress = main_PG[pt_index].page_number * PAGE_SIZE;
    free_kpages(vaddress);

    /*---------------------------------- SF LIST UPDATE -------------------------------------------------*/
    sf_list[index].p_number = main_PG[pt_index].page_number; 
    sf_list[index].p_pid = main_PG[pt_index].pid; 

}

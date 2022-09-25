/* code for manipulating the tlb (including replacement) */
#include <vm.h>
#include <spl.h>
#include <types.h>
#include <machine/tlb.h>
#include <vm_tlb.h>
#include <lib.h>
#include <pt.h>

extern unsigned int PAGETABLE_ENTRY;
extern struct invertedPT (*main_PG);
uint32_t freeTLBEntries = NUM_TLB - 1;
/**
* This method is used to add an entry to the hardware TLB.
*
* The physical address is taken from the page table. 
* If the Virtual Address and PID is not in the TLB, the kernel crash
*
* @author @InsideFra
* @param vaddr The virtual address (0x00..) (even not aligned)
* @param pid The process pid
* @param Dirty Specify if the segment is writable (== 1) or not (== 0)
* @date 02/08/2022
* @return 0 if everything is okay else panic
*/
int
addTLB(vaddr_t vaddr, pid_t pid, _Bool Dirty) {
    uint32_t ehi, elo;
    paddr_t pa, paddr = 0;
    int32_t tlb_index_probe;
    uint32_t page_index = vaddr/PAGE_SIZE;

    for (unsigned int i = 0; i < PAGETABLE_ENTRY; i++) {
        if (main_PG[i].Valid == 0)
            continue;
        
        if (main_PG[i].page_number == page_index)
            if(main_PG[i].pid == pid) {
                paddr = i*PAGE_SIZE + MIPS_KSEG0;
            }
    }

    if (paddr == 0) {
        panic("Error in addTLB from pageSearch");
        print_page_table();
    }

    ehi = vaddr & PAGE_FRAME; // PAGE ALIGN
    pa = paddr - MIPS_KSEG0;
    if (Dirty) 
        elo = pa | TLBLO_VALID | TLBLO_DIRTY;
    else
        elo = pa | TLBLO_VALID;

    int32_t spl = splhigh();
    tlb_index_probe = tlb_probe(ehi, elo); 
    if (tlb_index_probe > 0) {
        panic("TLB Error: duplicate TLB entries\n");
        return 1;
    }

    for (unsigned int i = 0; i < NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);

        if(! ((elo & 0x00000fff) & TLBLO_VALID)) { // invalid
            ehi = vaddr & PAGE_FRAME; // PAGE ALIGN
            pa = paddr - MIPS_KSEG0;
            elo = pa | TLBLO_VALID | TLBLO_DIRTY;
            tlb_write(ehi, elo, i);

            tlb_index_probe = tlb_probe(ehi, elo); 
            if (tlb_index_probe < 0) {
                panic("Generic TLB Error\n");
            }

            DEBUG(DB_TLB, "(TLBwrite ): [%3d] PN: %x\tpAddr: 0x%x\n", tlb_index_probe, ehi >> 12, paddr);

            freeTLBEntries--;

            splx(spl);
            return 0;
        }
    }
     
    ehi = vaddr & PAGE_FRAME; // PAGE ALIGN
    pa = paddr - MIPS_KSEG0;
    elo = pa | TLBLO_VALID | TLBLO_DIRTY;
    tlb_random(ehi, elo);

    tlb_index_probe = tlb_probe(ehi, elo); 
    if (tlb_index_probe < 0) {
        panic("Generic TLB Error\n");
    }

    DEBUG(DB_TLB, "(TLBreplac): [%3d] PN: %x\tpAddr: 0x%x\n", tlb_index_probe, ehi >> 12, paddr);

    splx(spl);
    return 0;

    // Yiu should not get here
    panic("No tlb entry available\n");
    return 1;
}
/**
* This method is used to remove an entry to the hardware TLB.
* @author @InsideFra
* @date 02/08/2022
* @param {vaddr_t} vaddr The virtual address (0x00..) (even not aligned)
* @return 0 if everything is entry is missing/the entry has been removed, 1 if error
*/
int
removeTLB(vaddr_t vaddr) {
    uint32_t ehi, elo;
    uint32_t paddr;
    vaddr &= PAGE_FRAME; // PAGE ALIGN

    uint32_t spl = splhigh();
    for (unsigned int i = 0; i < NUM_TLB; i++) {
        tlb_read(&ehi, &elo, i);

        if(ehi == vaddr) {
            paddr = elo >> 12;
            DEBUG(DB_TLB, "(TLBremove): [%3d] PN: %x\tpAddr: 0x%x\t", i, vaddr >> 12, PADDR_TO_KVADDR(paddr));
            freeTLBEntries++;

            ehi = TLBHI_INVALID(i);
            elo = TLBLO_INVALID();
            tlb_write(ehi, elo, i);
            
            if( (elo & 0x00000fff) & TLBLO_VALID) {
                DEBUG(DB_TLB, "-N- "); }
            else {
                DEBUG(DB_TLB, "-V- "); }

            if( (elo & 0x00000fff) & TLBLO_DIRTY) {
                DEBUG(DB_TLB, "D   -"); }
            else {
                DEBUG(DB_TLB, "-ND  -"); }
            
            DEBUG(DB_TLB, "\n");
            
            splx(spl);
            return 0;
        }
    }
    DEBUG(DB_TLB, "removeTLB: noEntryFound\t(vAddr: 0x%x)\n", (uint32_t)vaddr);
    splx(spl);
    return 0;
}
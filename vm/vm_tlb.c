/* code for manipulating the tlb (including replacement) */
#include <vm.h>
#include <spl.h>
#include <types.h>
#include <machine/tlb.h>
#include <vm_tlb.h>
#include <lib.h>

/**
* This method is used to add an entry to the hardware TLB.
* @author @InsideFra
* @param vaddr The virtual address (0x00..) (even not aligned)
* @param paddr The physicial address (0x80..)
* @param Dirty Specify if the segment is writable (== 1) or not (== 0)
* @date 02/08/2022
* @return 0 if everything is okay else panic
*/
int
addTLB(vaddr_t vaddr, paddr_t paddr, _Bool Dirty) {
    uint32_t ehi, elo;
    paddr_t pa;
    int32_t tlb_index_probe;

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

            DEBUG(DB_VM, "(TLBwrite ): [%3d] PN: %x\tpAddr: 0x%x\n", tlb_index_probe, ehi >> 12, paddr);

            splx(spl);
            return 0;
        }
    }
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
            ehi = TLBHI_INVALID(i);
            elo = TLBLO_INVALID();
            tlb_write(ehi, elo, i);

            DEBUG(DB_VM, "(TLBremove): [%3d] PN: %x\tpAddr: 0x%x\t", i, vaddr >> 12, PADDR_TO_KVADDR(paddr << 12));
            
            if( (elo & 0x00000fff) & TLBLO_DIRTY) {
                DEBUG(DB_VM, "Dirty "); }
            else {
                DEBUG(DB_VM, "No Dirty "); }
            
            if( (elo & 0x00000fff) & TLBLO_VALID) {
                DEBUG(DB_VM, "Invalid "); }
            else {
                DEBUG(DB_VM, "Valid "); }

            DEBUG(DB_VM, "\n");
            
            splx(spl);
            return 0;
        }
    }
    DEBUG(DB_VM, "removeTLB Error: noEntryFound\n");
    splx(spl);
    return 1;
}
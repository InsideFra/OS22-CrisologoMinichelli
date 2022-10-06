/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _VM_H_
#define _VM_H_

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */

#include <types.h>
#include <machine/vm.h>
#include <mips/specialreg.h>
#include <lib.h>
#include <mips/trapframe.h>


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/

////////////////////////////////////////

#if PAGE_SIZE == 4096

#define NSIZES 8
static const size_t sizes[NSIZES] = { 16, 32, 64, 128, 256, 512, 1024, 2048 };

#define SMALLEST_SUBPAGE_SIZE 16
#define LARGEST_SUBPAGE_SIZE 2048

#elif PAGE_SIZE == 8192
#error "No support for 8k pages (yet?)"
#else
#error "Odd page size"
#endif

#define MAX_PAGES_PER_PROCESS 190
#define MAX_CODE_SEGMENT_PAGES 4
#define MAX_DATA_SEGMENT_PAGES 150
#define MAX_STACK_SEGMENT_PAGES 4

#define MAX_PAGES_ALLOC 10

////////////////////////////////////////


/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, struct trapframe *tf);

paddr_t alloc_kpages(unsigned npages);
void free_kpages(paddr_t paddr);

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown(const struct tlbshootdown *);

paddr_t alloc_pages(uint8_t npages, vaddr_t vaddr, bool Dirty);

// for debug purpose
void print_page_table(void);
void print_frame_list(void);
void print_vm_stat(void);

#endif /* _VM_H_ */

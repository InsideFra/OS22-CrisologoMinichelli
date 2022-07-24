# OS22-CrisologoMinichelli
 Project C1 made by Minichelli and Crisologo for the OS Course at PoliTO

 ## SYS161

 To be done..

 ## Memory Organization of SYS161

 The memory organization of SYS161 can be found in the include file [vm.h](/arch/mips/include/vm.h).
 
 The address bus has a width of 32bit, thus giving an ideal address space of 4GB. 
 
 In reality, the physical memory is much lower, and it composed of a physical ram, which size can be set in the config file "sys161.conf" in the "/root" directory. By now this project is configured to have 512kB of RAM.

 - kuseg
   - 0x00000000 - 0x7fffffff (user, tlb-mapped).
   - Those addresses should only be used in the user-mode.
   - When trying to read or write in this address space, the TLB-Hardware mechanism kicks in. There is the need to map those addresses to the physical memory.
   - This address space can be used to virtualize memory and load programs which have a size higher than the real physical memory.
 - kseg0
   - 0x80000000 - 0x9fffffff (kernel, unmapped, cached).
   - We can find the physical RAM in this address space.
   - MIPS_KSEG0 = 0x80000000.
   - Physical RAM starts from 0x80000000 to size_t lastpaddr+MIPS_KSEG0, which can be retreived by using the function size_t mainbus_ramsize().
   - The physical RAM taken by the kernel during startup starts from 0x80000000 to vaddr_t firstfree (set by start.S).
 - kseg1
   - We are not talking about this by now.
 - kseg2
   - We are not talking about this by now. 

 ## Project Description
 Project c1: Virtual Memory with Demand Paging (PAGING)

 Virtual Memory is the separation of the user logical memory from physical memory. Virtual memory can be implemented via:
 - Demand paging
 - Demand segmentation

 ### Demand Paging

 ```c
 To be done..
 ```

 ### TLB - Translation look-aside buffers (TLBs)
 
 Also called associative memory.

 ## New TLB Implementation

 To be done..

 ### TLB Replacement policy

 To be done..

 ### runprogram 

 In order to start a program, the int runprogram(*char progname) function is called. In the os161 implementation, the function follow the following state flow:

 ```mermaid
 graph TD;
   A[runprogram]-->B;
   B[vfs_open]-->C;
   C[as_create]-->D;
   D[switch & activate address space]-->E[load_elf];
 ```
 ### Round Robin Scheduling
 
 For this project, a Round-Robin (RR) like algorith is used to schedule the upcoming processes.

 This algorithm is based on the time-sharing technique, giving for each job a time-slot also called time quantum. 

 ## Dumbvm
 
 Dumbvm is the embedded Virtual Memory Managment of the OS161. One of the goal of this project is to replace this architecture with a new virtual-memory system that relaxes some (not all) of dumbvs's limitations.

 ## Snippets used during coding
 
 While using VSCode, just press Ctrl+Shitf+V in order to display the Markdown Preview of the opened .md file.

 ## How to debug with visual studio?
 
 ```bash
 cd /home/francesco/os161/root
 sys161 -w kernel
 ```
 Then in VSCode, just press F5.

 ## Error debugger messages

 > sys161: trace: software-requested debugger stop

 If you read this message in the console probably there is a problem with spinlock (a function is taking to long to execute)

 > sys161: disk: slot 2: LHD0.img: Locked by another process

 If you read this message in the console while trying to execute sys161, execute:

 ```bash
 sudo kill -9 $(lsof -t LHD0.img)
 ```


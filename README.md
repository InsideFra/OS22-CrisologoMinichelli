# OS22-CrisologoMinichelli
 Project C1 made by Minichelli and Crisologo for the OS Course at PoliTO

 ## Project Description
 Project c1: Virtual Memory with Demand Paging (PAGING)

 Virtual Memory is the separation of the user logical memory from physical memory. Virtual memory can be implemented via:
 - Demand paging
 - Demand segmentation

  ### TLB - Translation look-aside buffers (TLBs)
  Also called associative memory.

  ### TLB Replacement policy

  ### runprogram 
  In order to start a program, the int runprogram(*char progname) function is called. In the os161 implementation, the function follow the following state flow:

  ```mermaid
  graph TD;
    A[runprogram]-->B;
    B[vfs_open]-->C;
    C[as_create]-->D;
    D[switch & activate address space]-->E[load_elf];
  ```
  Actually as_create is just a kmalloc function call.

  ```c
  //Allocate a block of size SZ. Redirect either to subpage_kmalloc or alloc_kpages depending on how big SZ is.
  void * kmalloc(size_t sz);
  ```


  ### Demand Paging

  ```c
  
  ```

  ### Round Robin Scheduling
  For this project, a Round-Robin (RR) like algorith is used to schedule the upcoming processes.

  This algorithm is based on the time-sharing technique, giving for each job a time-slot also called time quantum. 

 ## Dumbvm
 Dumbvm is the embedded Virtual Memory Managment of the OS161. One of the goal of this project is to replace this architecture with a new virtual-memory system that relaxes some (not all) of dumbvs's limitations.

 ## New TLB Implementation

 ## Snippets used during coding
 While using VSCode, just press Ctrl+Shitf+V in order to display the Markdown Preview of the opened .md file.

 ## How to debug with visual studio?
 ```bash
 cd /home/francesco/os161/root
 sys161 -w kernel
 ```
 Then in VSCode, just press F5.


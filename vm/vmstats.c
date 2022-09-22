/* code for tracking stats */


unsigned int TLB_Faults = 0; // The number of TLB misses that have occurred (not including faults that cause a program to crash)
unsigned int TLB_Faults_wFree = 0;
unsigned int TLB_Faults_wReplace = 0;
unsigned int TLB_Invalidations = 0;
unsigned int TLB_Reloads = 0;

unsigned int PF_Zeroed = 0;
unsigned int PF_Disk = 0;
unsigned int PF_ELF = 0;
unsigned int PF_Swapfile = 0;
unsigned int SF_Writes = 0;
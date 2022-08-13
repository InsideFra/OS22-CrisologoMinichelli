#include <types.h>
#include <syscall.h>

/**
* This method tries to emulate the write in c system call.
* @author @InsideFra
* @param {int} fd The virtual address (0x00..)
* @param {vaddr_t} vaddr the address space
* @param {uint} size the amount of bytes to writw
* @date 12/08/2022
* @return 1 if everything is okay else ..
*/
int
sys_write(int fd, vaddr_t vaddr, uint32_t size)
{
  (void)fd;
  (void)vaddr;
  (void)size;
  return 0;
}

#include "common.h"
#include "syscall.h"

#include "fs.h"

extern int mm_brk(uint32_t new_brk);

// pa3-2-1
// int sys_write(int fd, const void *buf, size_t len){
//   Log("sys_write:fd %d  len %d\n",fd,len);
//   if (fd == 1 || fd == 2) {
//     int i = 0;
//     for(; i < len; i++){
//       _putc(((char*)buf)[i]);
//     }
//     return i;
//   }
//   return -1;
// }

_RegSet* do_syscall(_RegSet *r) {
  uintptr_t a[4];
  a[0] = SYSCALL_ARG1(r);
  a[1] = SYSCALL_ARG2(r);
  a[2] = SYSCALL_ARG3(r);
  a[3] = SYSCALL_ARG4(r);

  switch (a[0]) {
    case SYS_none:
      SYSCALL_ARG1(r) = 1;
      break;
    case SYS_exit:
      _halt(a[1]);
      break;
    case SYS_write:
      // SYSCALL_ARG1(r) = sys_write((int)a[1],(void *)a[2],(size_t)a[3]);
      SYSCALL_ARG1(r) = fs_write((int)a[1],(void *)a[2],(size_t)a[3]);
      break;
    case SYS_brk:
      // SYSCALL_ARG1(r) = 0; // pa3
      SYSCALL_ARG1(r) = mm_brk(a[1]); // pa4
      break;
    case SYS_open:
      SYSCALL_ARG1(r) = fs_open((char *)a[1],(int)a[2],(int)a[3]);
      break;
    case SYS_read:
      SYSCALL_ARG1(r) = fs_read((int)a[1],(void *)a[2],(size_t)a[3]);
      break;
    case SYS_lseek:
      SYSCALL_ARG1(r) = fs_lseek((int)a[1],(off_t)a[2],(int)a[3]);
      break;
    case SYS_close:
      SYSCALL_ARG1(r) = fs_close((int)a[1]);
      break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }

  return NULL;
}

#include "common.h"
#include "fs.h"
#include "memory.h"

// #define DEFAULT_ENTRY ((void *)0x4000000)
#define DEFAULT_ENTRY ((void *)0x8048000)

size_t get_ramdisk_size();
void ramdisk_read(void *buf, off_t offset, size_t len);

uintptr_t loader(_Protect *as, const char *filename) {
  // TODO();
  // pa3 part1
  // ramdisk_read(DEFAULT_ENTRY, 0, get_ramdisk_size());
  // pa3 part2
  // int fd=fs_open("/bin/text",0,0);
  // pa3 part3-1
  // int fd=fs_open("/bin/bmptest",0,0);
  // pa3 part3-2
  // int fd=fs_open("/bin/events",0,0);
  
  // pa3 part3-3
  // int fd=fs_open("/bin/pal",0,0);
  // int size=fs_filesz(fd);
  // fs_read(fd,DEFAULT_ENTRY,size);
  // return (uintptr_t)DEFAULT_ENTRY;

  // pa4-1
  // int fd=fs_open("/bin/dummy",0,0);
  // int fd=fs_open("/bin/pal",0,0);
  int fd=fs_open(filename,0,0);
  int size=fs_filesz(fd);
  void *page;

  for(int i=0;i<size;i+=PGSIZE){
    page = (void*)new_page();
    _map(as, DEFAULT_ENTRY + i, page);//映射到物理地址
    fs_read(fd, page, PGSIZE);//读一页文件
  }
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}

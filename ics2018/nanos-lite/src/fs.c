#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;//文件打开后的读写指针
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void ramdisk_read(void *buf, off_t offset, size_t len);
void ramdisk_write(const void *buf, off_t offset, size_t len);
void dispinfo_read(void *buf, off_t offset, size_t len);
void fb_write(const void *buf, off_t offset, size_t len);
size_t events_read(void *buf, size_t len);

void init_fs() {
  // TODO: initialize the size of /dev/fb
  file_table[FD_FB].size=_screen.width*_screen.height*sizeof(uint32_t);
}

size_t fs_filesz(int fd) {
  return file_table[fd].size;
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {//查找文件
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  assert(0);//没有该文件
  return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  if (fd == FD_EVENTS){//事件
    return events_read(buf, len);
  }
  int toRead = (file_table[fd].size - file_table[fd].open_offset > len) ? len : file_table[fd].size - file_table[fd].open_offset;//要读出的长度
  if (toRead > 0) {
    if (fd == FD_DISPINFO) {//屏幕
      dispinfo_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, toRead);//读文件
    }
    else {
      ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, toRead);//读文件
    }
    file_table[fd].open_offset += toRead;//修改偏移
  }
  return toRead;
}


ssize_t fs_write(int fd, const void *buf, size_t len) {
  int toWrite = (file_table[fd].size - file_table[fd].open_offset > len) ? len : file_table[fd].size - file_table[fd].open_offset;//要写的长度
  switch(fd) {
    case FD_STDOUT:
    case FD_STDERR: {//输出到串口
      toWrite = len;
      for(int i = 0; i < len; i++){
        _putc(((char*)buf)[i]);
      }
      break;
    }
    case FD_NORMAL: {//输出文件内容
      if (toWrite > 0) {
        ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, toWrite);//写文件
      }
      break;
    }
    case FD_FB: {
      fb_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, toWrite);
      break;
    }
    case FD_EVENTS: {
      toWrite = len;
      break;
    }
    default: {
      // assert(0);
      ramdisk_write(buf,file_table[fd].disk_offset+file_table[fd].open_offset,toWrite);
      break;
    }
  }
  file_table[fd].open_offset += toWrite;//修改偏移
  return toWrite;
}

 
off_t fs_lseek(int fd, off_t offset, int whence){
  int newOfset = 0;
  switch (whence) {
    case SEEK_SET: {
      newOfset = offset;
      break;
    }
    case SEEK_CUR: {
      newOfset = file_table[fd].open_offset + offset;
      break;
    }
    case SEEK_END: {
      newOfset = file_table[fd].size + offset;
      break;
    }
  }
  if (newOfset < 0){
    newOfset = 0;
  }
  else if (newOfset > file_table[fd].size){
    newOfset = file_table[fd].size;
  }
  file_table[fd].open_offset = newOfset;
  return newOfset;
}

int fs_close(int fd) {
  //没有维护文件打开的操作，直接返回0
  return 0;
}



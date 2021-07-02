#include "common.h"

/* Uncomment these macros to enable corresponding functionality. */
#define HAS_ASYE
#define HAS_PTE

void init_mm(void);
void init_ramdisk(void);
void init_device(void);
void init_irq(void);
void init_fs(void);
uint32_t loader(_Protect *, const char *);

extern void load_prog(const char*);

int main() {
#ifdef HAS_PTE
  init_mm();
#endif

  Log("'Hello World!' from Nanos-lite");
  Log("Build time: %s, %s", __TIME__, __DATE__);

  init_ramdisk();

  init_device();

#ifdef HAS_ASYE
  Log("Initializing interrupt/exception handler...");
  init_irq();
#endif

  init_fs();

  // uint32_t entry = loader(NULL, "/bin/pal");
  // ((void (*)(void))entry)();

  // pa4-part1
  // load_prog("/bin/dummy");
  load_prog("/bin/pal");
  // pa4-part2
  load_prog("/bin/hello");
  // pa4-part4
  load_prog("/bin/videotest");
  _trap();

  panic("Should not reach here");
}
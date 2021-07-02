#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  // _switch(&pcb[i].as);
  // current = &pcb[i];
  // ((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}


static PCB *current_game = &pcb[0];
void switch_game() {
    current_game = (current_game == &pcb[0] ? &pcb[2] : &pcb[0]);
}
  

_RegSet* schedule(_RegSet *prev) {
  // return NULL;
  static int count = 0;
  current->tf = prev;//保存当前上下文指针
  // current = &pcb[0];//选择下一个要执行的进程 pa4-part2-2
  // current=(current == &pcb[0] ? &pcb[1] : &pcb[0]); // pa4-part2-3
  if (count < 1000) {// pa4-part2-4
    // current = &pcb[0];
    current = current_game;// pa4-part4
    count++;
  }
  else {
    current = &pcb[1];
    count = 0;
  }
  _switch(&current->as);//切换
  return current->tf;
}

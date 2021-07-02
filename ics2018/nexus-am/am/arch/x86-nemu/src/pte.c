#include <x86.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      // fill PDE
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs);
  set_cr0(get_cr0() | CR0_PG);
}

void _protect(_Protect *p) {
  PDE *updir = (PDE*)(palloc_f());
  p->ptr = updir;
  // map kernel space
  for (int i = 0; i < NR_PDE; i ++) {
    updir[i] = kpdirs[i];
  }

  p->area.start = (void*)0x8000000;
  p->area.end = (void*)0xc0000000;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}


void _map(_Protect *p, void *va, void *pa) {
	PDE* basePDE = (PDE*) p->ptr;//基地址
	
	uint32_t dir = PDX(va);
	uint32_t page = PTX(va);
	if(!(basePDE[dir] & PTE_P)){//检查present位是否有效
		PTE* basePTE = (PTE*)palloc_f();//申请空闲物理页
		basePDE[dir] = (uint32_t)basePTE | PTE_P;//设置present位有效
		basePTE = (PTE*)(basePDE[dir] & 0xfffff000);
		basePTE[page] = (uint32_t)pa | PTE_P;//映射到pa
	}
	else{
		PTE* basePTE = (PTE*)(basePDE[dir] & 0xfffff000);
		basePTE[page] = (uint32_t)pa | PTE_P;//映射到pa
	}
}

void _unmap(_Protect *p, void *va) {
}

_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  // return NULL;
  // 初始化陷阱帧
	uint32_t* ptr = (uint32_t*)(ustack.end-4);
	*(ptr--)=0;
	*(ptr--)=0;
	*(ptr--)=0;
	*(ptr--)=0;// 设置_start栈帧

	_RegSet* tf = (void*)(ptr-sizeof(_RegSet));
	tf->cs=8;// 使diff-test正常运行
	tf->eip=(uintptr_t)entry;
	tf->eflags = 0x20 | FL_IF;// pa4-part3 打开用户进程中断
	
  return tf;
}

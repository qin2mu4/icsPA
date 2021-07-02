#include "nemu.h"
#include "device/mmio.h"
#include "memory.h"

#define PMEM_SIZE (128 * 1024 * 1024)

// pa4
#define PTE_P     0x001     // Present
#define PTE_A     0x020     // Accessed
#define PTE_D     0x040     // Dirty


#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  // return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  int isPhyAdd = is_mmio(addr);//获取内存映射号
  if (isPhyAdd != -1) {
    return mmio_read(addr, len, isPhyAdd);//被映射到 I/O 空间
  }
  else {
    return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  }
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  // memcpy(guest_to_host(addr), &data, len);
  int isPhyAdd = is_mmio(addr);//获取内存映射号
  if (isPhyAdd != -1) {
    mmio_write(addr, len, data, isPhyAdd);//被映射到 I/O 空间
  }
  else {
    memcpy(guest_to_host(addr), &data, len);
  }
}

paddr_t page_translate(vaddr_t addr,bool iswrite){
  paddr_t pde;//页目录
  paddr_t pte;//页
  paddr_t base1 = cpu.cr3.val;//页目录基地址
  paddr_t pde_address=base1 + ((addr >> 22) << 2);//页目录地址
  pde=paddr_read(pde_address,4);//页目录
  if(!(pde & PTE_P)) {//检查present位，是无效页目录
    assert(0);
  } 
  paddr_t base2=pde & 0xfffff000;//页表基地址
  paddr_t pte_address=base2 + ((addr & 0x003ff000) >> 10);//页表地址
  pte =paddr_read(pte_address,4);//页表
  if(!(pte & PTE_P)) {//检查present位，是无效页表
    assert(0); 
  }
      
  paddr_t paddress=(pte&0xfffff000)+(addr&0xfff);//实际地址

  // 设置accessed和dirty位
  pde = pde | PTE_A;
  pte = pte | PTE_A;
  if (iswrite) {
    pde = pde | PTE_D;
    pte = pte | PTE_D;
  }
  paddr_write(pde_address,4,pde);
  paddr_write(pte_address,4,pte); 
  return paddress;//返回实际地址
}  


uint32_t vaddr_read(vaddr_t addr, int len) {
  // return paddr_read(addr, len);
  if(cpu.cr0.paging){//分页机制开启
    if(((addr & 0xfff) + len) > PAGE_SIZE){//数据跨越页边界
      // assert(0);//pa4-1
      paddr_t paddr,low,high;
      int x;
      x=(int)((addr&0xfff)+len-4096);

      paddr=page_translate(addr,false);
      low=paddr_read(paddr,len-x);//读前一个页

      paddr=page_translate(addr+len-x,false);
      high=paddr_read(paddr,x);//读后一个页

      paddr=(high<<((len-x)<<3))+low;//拼接

      return paddr;
    } 
    else  {
      paddr_t paddr = page_translate(addr, false);
      return paddr_read(paddr, len);
    } 
  }  
  else{//分页机制关闭
    return paddr_read(addr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  // paddr_write(addr, len, data);
  if(cpu.cr0.paging){//分页机制开启
    if(((addr & 0xfff) + len) > PAGE_SIZE){//数据跨越页边界
      //assert(0); //pa4-1
      paddr_t paddr,low,high;
      int x;
      x=(int)((addr&0xfff)+len-0x1000);

      low=(data<<(x<<3))>>(x>>3);//前一个页中写入的数据长度
      paddr=page_translate(addr,true);//转换
      paddr_write(paddr,len-x,low);//在前一个页中写入

      high=data>>((len-x)<<3);//后一个页中写入的数据长度
      paddr=page_translate(addr+len-x,true);//转换
      paddr_write(paddr,x,high);//在后一个页中写入
    }   
    else{
      paddr_t paddr = page_translate(addr, true);
      paddr_write(paddr, len, data);
    }   
  }
  else{//分页机制关闭
    paddr_write(addr, len, data); 
  }
}

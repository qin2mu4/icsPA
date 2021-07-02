#include "cpu/exec.h"
#include "memory/mmu.h"
#include "cpu/reg.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  // TODO();
  rtl_push(&(cpu.eflags.eflagsVal));
  rtl_push((rtlreg_t*)&(cpu.cs));
  rtl_push(&ret_addr);//保存现场

  uint32_t idtr_base = cpu.idtr.base;//读首地址
  uint32_t eip_low, eip_high, offset;
  eip_low=vaddr_read(idtr_base+NO*8,4)&0x0000ffff;
  eip_high = vaddr_read(idtr_base + NO * 8 + 4, 4) & 0xffff0000;//索引
  offset = eip_low | eip_high;//目标地址
	decoding.jmp_eip = offset;//设置跳转
	decoding.is_jmp = true;
  cpu.eflags.IF = 0;// pa4-part3关中断
}

void dev_raise_intr() {
  cpu.INTR = true;//设为高电平
}

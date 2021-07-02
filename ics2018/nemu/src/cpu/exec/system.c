#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();
void raise_intr(uint8_t,vaddr_t);

make_EHelper(lidt) {
  // TODO();
  cpu.idtr.limit=vaddr_read(id_dest->addr,2);
  if(decoding.is_operand_size_16) {
    cpu.idtr.base=vaddr_read(id_dest->addr+2,4)&0x00ffffff;
  }
  else{
    cpu.idtr.base=vaddr_read(id_dest->addr+2,4);
  }
  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  // TODO();
  switch(id_dest->reg){
    case 0: cpu.cr0.val=reg_l(id_src->reg); break;
    case 3: cpu.cr3.val=reg_l(id_src->reg); break;
    default: assert(0);
  }
  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  // TODO();
  switch(id_src->reg) {
    case 0: reg_l(id_dest->reg)=cpu.cr0.val; break;
    case 3: reg_l(id_dest->reg)=cpu.cr3.val; break;
    default: assert(0);
  }
  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  // TODO();
  raise_intr(id_dest->val,decoding.seq_eip);
  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  // TODO();
  rtl_pop(&t2);
  decoding.jmp_eip=t2;
  rtl_pop((rtlreg_t *)&cpu.cs);
  rtl_pop(&(cpu.eflags.eflagsVal));//恢复现场
  decoding.is_jmp=1;//设置跳转
  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  // TODO();
  t1 = pio_read(id_src->val, id_dest->width);
  operand_write(id_dest, &t1);//写回
  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  // TODO();
  pio_write(id_dest->val, id_dest->width, id_src->val);
  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

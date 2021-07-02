#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

#define NR_WP 32//最多设置的监视点个数
#define MAX_EXPR_LEN 128//表达式

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */

  char expr[MAX_EXPR_LEN];//用于储存表达式
  uint32_t lastVal;//上一条指令执行之后的值
} WP;

WP* new_wp();
void free_wp(int id);
void printWatchpointInfo();
bool checkWatchpoint();

#endif

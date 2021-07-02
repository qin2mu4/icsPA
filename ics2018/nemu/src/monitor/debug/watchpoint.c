#include "monitor/watchpoint.h"
#include "monitor/expr.h"



static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

//获取一个空的监视点
WP* new_wp() {
  if (free_ == NULL) {
    printf("There is no more watchpoint.");
    return NULL;
  }
  WP* tempWP = free_;
  free_ = free_->next;//获取一个未使用监视点
  tempWP->next = head;
  head = tempWP;//加入已使用列表
  printf("Get a new watchpoint with id %d.\n", tempWP->NO);
  return tempWP;
}

//删除监视点
void free_wp(int id) {
  WP* temp = head;
  if (head->NO == id){
    //head是删除的监视点
    head = head->next;
    temp->next = free_;
    free_ = temp;
    printf("Watchpoint with id %d has been deleted.\n", id);
    return;
  }
  else {
    //查找要删除的监视点
    WP* curPos = head;
    while(curPos->next != NULL) {
      if (curPos->next->NO == id) {
        temp = curPos->next;
        curPos->next = curPos->next->next;
        temp->next = free_;
        free_ = temp;
        printf("Watchpoint with id %d has been deleted.\n", id);
        return;
      }
      curPos = curPos->next;
    }
  }
  printf ("Watchpoint with id %d has not been set.\n", id);
}

//输出监视点信息
void printWatchpointInfo(){
  WP* curPos = head;
  if (head == NULL){
    printf("There is no watchpoint set.\n");
    return;
  }
  int num = 0;
  while (curPos != NULL) {
    printf("Watchpoint id: %d\texpr: %s\tnow value: %d\n", curPos->NO, curPos->expr, curPos->lastVal);
    num++;
    curPos = curPos->next;
  }
  printf("total num: %d\n", num);
}

//检查监视点是否变化
bool checkWatchpoint(){
  bool isChange = false;
  WP* curPos = head;
  while (curPos != NULL) {
    bool success = true;
    uint32_t curVal = expr(curPos->expr, &success);
    if (success == false) {
      printf("cpu-exec.c watchpoint which id %d wrong expr.\n", curPos->NO);
    }
    else if (curPos->lastVal != curVal){
      isChange = true;
      curPos->lastVal = curVal;
      printf("A watchpoint which id is %d and expr is '%s' has been triggered, now value is %d.\n", 
            curPos->NO, curPos->expr, curPos->lastVal);
    }
    curPos = curPos->next;
  }
  return isChange;
} 
#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    cpu_exec(1);
  }
  else {
    int n = 0;
    for (int i = 0; args[i]; i++) {
      n = n * 10 + args[i] - 48;
    }
    cpu_exec(n);
  }
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("info r: print register status. info w: print monitor information.");
    return -1;
  } 
  else {
    switch (args[0]){
      case 'r': {
        //eax, ecx, edx, ebx, esp, ebp, esi, edi, eip
        printf("eax\t%#x\n", cpu.eax);
        printf("ecx\t%#x\n", cpu.ecx);
        printf("edx\t%#x\n", cpu.edx);
        printf("ebx\t%#x\n", cpu.ebx);
        printf("esp\t%#x\n", cpu.esp);
        printf("ebp\t%#x\n", cpu.ebp);
        printf("esi\t%#x\n", cpu.esi);
        printf("edi\t%#x\n", cpu.edi);
        printf("eip\t%#x\n", cpu.eip);
        return 0;
      }
      case 'w':{
        printWatchpointInfo();
        return 0;
      }
      default:{
        printf("info r: print register status. info w: print monitor information.\n");
        return -1;
      }
    }
  }
  return 0;
}

static int cmd_p(char *args) {
  char *e = strtok(NULL, " ");
  if (e == NULL) {
    printf("p EXPR. Find the value of EXPR. eg: p $eax+1\n");
    return -1;
  }
  bool success = true;
  uint32_t result = expr(e, &success);
  if (success == false) {
    printf("'ui.c' wrong expr.\n");
    return -1;
  }
  else {
    printf("%d\n", result);
    return 0;
  }
}

static int cmd_x(char *args) {
  char *chOutNum = strtok(NULL, " ");
  char *e = strtok(NULL, " ");
  if (chOutNum == NULL || e == NULL) {
    printf("x N EXPR. Find out the value of EXPR, take the result as the starting memory address, and output n consecutive 4 bytes in hexadecimal form the starting memory. eg: x 10 $esp\n");
    return -1;
  }
  //计算开始输出的地址
  bool success = true;
  uint32_t addr = expr(e, &success);
  if (success == false) {
    printf("'ui.c' wrong expr.\n");
    return -1;
  }

  //计算输出的字节数并输出连续的N个4字节
  int numLen = strlen(chOutNum);
  int intOutNum = 0;//N
  for (int i = 0; i < numLen; i++) {
    intOutNum = intOutNum * 10 + chOutNum[i] - 48;
  }
  printf("%#x\t%#x\n", addr, vaddr_read(addr, 4*intOutNum));
  return 0;
}

static int cmd_w(char *args) {
  char *e = strtok(NULL, " ");
  if (e == NULL) {
    printf("w EXPR. Suspend the program execution when the value of expr changes. eg: w *0x20000\n");
    return -1;
  }
  
  //计算表达式值
  if (strlen(e) > MAX_EXPR_LEN) {//表达式太长
    printf("The expr is too long, max %d\n", MAX_EXPR_LEN);
    return -1;
  }
  bool success = true;
  int lastVal = expr(e, &success);
  if (success == false) {//表达式有误
    printf("ui.c wrong expr\n");
    return -1;
  }

  //申请和初始化监视点
  WP* wp = new_wp();
  if (wp != NULL) {
    strcpy(wp->expr, e);
    wp->lastVal = lastVal;
    return 0;
  }
  return -1;
}

static int cmd_d(char *args){
  char* numStr = strtok(NULL, " ");
  if (numStr == NULL) {
    printf("d N. Delete monitor point with serial number n. eg: d 2\n");
  }

  //计算要删除的监视点id
  int numLen = strlen(numStr);
  int watchpointID = 0;
  for (int i = 0; i < numLen; i++){
    watchpointID = watchpointID * 10 + numStr[i] - '0';
  }
  if (watchpointID > NR_WP) {
    printf("Watchpoint id should be less than %d.\n", NR_WP);
    return -1;
  }

  //删除监视点
  free_wp(watchpointID);
  return 0;
}

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "si [N]. Pause the program after N instructions are executed in one step. When n is not given, it defaults to 1.  eg: si 10", cmd_si},
  { "info", "info r: print register status. info w: print monitor information.", cmd_info },
  { "p", "p EXPR. Find the value of EXPR. eg: p $eax+1", cmd_p },
  { "x", "x N EXPR. Find out the value of EXPR, take the result as the starting memory address, and output n consecutive 4 bytes in hexadecimal form the starting memory. eg: x 10 $esp", cmd_x },
  { "w", "w EXPR. Suspend the program execution when the value of expr changes. eg: w *0x20000", cmd_w },
  { "d", "d N. Delete monitor point with serial number n. eg: d 2", cmd_d }

};


static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256,

  /* TODO: Add more token types */
  TK_NUM,//数字
  TK_LBRACKET, TK_RBRACKET, //（ ）
  TK_ADD, TK_SUB, TK_MUL, TK_DIV, //+-*/
  TK_DEREFERENCE, TK_MINUS, //解引用，负号
  TK_G, TK_L, TK_GE, TK_LE, //> < >= <= 
  TK_EQ, TK_NEQ, // == != 
  TK_AND, TK_OR, TK_NOT, // && || !
  TK_REG,//寄存器
  TK_ID//变量标识符
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  //@还有部分未添加
  {" +", TK_NOTYPE},    // spaces
  {"0|[1-9][0-9]*", TK_NUM},  
  {"\\$(e[a,b,c,d]x|e[s,b,i]p|e[s,d]i)", TK_REG},
  {"[a-zA-Z][0-9a-zA-Z_]*", TK_ID},
  {"\\(", TK_LBRACKET},   
  {")", TK_RBRACKET},
  {">=", TK_GE}, 
  {">=", TK_LE}, 
  {"==", TK_EQ},
  {"!=", TK_NEQ},   
  {"!", TK_NOT},
  {"\\*", TK_MUL},      
  {"\\/", TK_DIV},      
  {"\\+", TK_ADD},      
  {"-", TK_SUB},       
  {">", TK_G}, 
  {"<", TK_L}, 
  {"&&", TK_AND},
  {"||", TK_OR}      
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

#define MAX_TOKEN_LEN 128 //字符串最长128
Token tokens[MAX_TOKEN_LEN];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;//tokens总数

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_NUM:
          case TK_ID:
          case TK_REG:
            // printf("test: make_token\n");
            if (substr_len <= MAX_TOKEN_LEN){//@不一定对
              // printf("test: make 1.\n");
              for (int j = 0; j < substr_len; j++) {
                tokens[nr_token].str[j] = *(substr_start+j);
              }
              // printf("test: make 1 down.\n");
            }
            else{
              // printf("test: make 2.\n");
              printf("The token is too long(need to be < %d).\n", MAX_TOKEN_LEN);
              // printf("test: make 2 down.\n");
              return false;
            }
            // printf("test: one copy down.\n");
            break;
        }
        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses(int p, int q, bool* success){//检查括号匹配
  if (tokens[p].type != TK_LBRACKET || tokens[q].type != TK_RBRACKET) {
    return false;
  }
  int simuStack = 0;//模拟栈顶
  for (int i = p; i <= q; i++) {
    if (tokens[i].type == TK_LBRACKET) {
      simuStack++;
    }
    else if (tokens[i].type == TK_RBRACKET) {
      simuStack--;
    }
    if (simuStack == 0 && i != q) { //最左最右括号不是同一对
      return false; 
    }
    if (simuStack < 0) {//表达式有误
      *success = false;
      return true; //为了在eval中停止递归，返回true
    }
  }
  if (simuStack != 0){//左括号数大于右括号，表达式有误
    *success = false;
  }
  return true;
}
 
int findDominantOp(int p, int q) {
  // printf("test: p %d q %d", p, q);
  int opType = -1, opPos = 0;// 0 取地址 负 !, 1 * /, 2 + -, 3 > < >= <=, 4 == !=, 5 &&, 6 ||
  for (int i = p; i <= q; i++) {
    int tempType = 0;//运算符类型
    switch (tokens[i].type) {
      case TK_DEREFERENCE:
      case TK_MINUS:
      case TK_NOT:
        tempType = 0;
        break;
      case TK_MUL:
      case TK_DIV:
        tempType = 1;
        break;
      case TK_ADD:
      case TK_SUB:
        tempType = 2;
        break;
      case TK_G:
      case TK_L:
      case TK_GE:
      case TK_LE:
        tempType = 3;
        break;
      case TK_EQ:
      case TK_NEQ:
        tempType = 4;
        break;
      case TK_AND:
        tempType = 5;
        break;
      case TK_OR:
        tempType = 6;
        break;
      case TK_LBRACKET:
        while (i <= q && tokens[i].type != TK_RBRACKET) {
          //因前面处理过括号，这里不会是一整个括号，且DominantOp不会在括号中，直接跳过括号包含的部分
          i++;
        }
      default:
        continue;
    }
    if (tempType > opType) {
      opType = tempType;
      opPos = i;
    }
  }
  return opPos;
}

//isNum用来指示是否是数字（可能是多位数字）
uint32_t eval(int p, int q, bool* success) {
  // printf("test: eval begin\n");
  if (p > q) {//表达式有误
    // printf("test: p>q\n");
    *success = false;
    return -1;
  }
  else if (p == q) {//单独的数字/寄存器/id
    // printf("test: p==q\n");
    if (tokens[p].type == TK_NUM) {//数字
      // printf("test: p==q TK_NUM\n");
      int numLen = strlen(tokens[p].str);
      int num = 0;
      for (int i = 0; i < numLen; i++) {
        num = num*10 + tokens[p].str[i] - '0'; 
      }
      return num;
    }
    else if (tokens[p].type == TK_REG) {//寄存器
      // printf("test: p==q TK_REG\n");
      // printf("test: %d %d\n", p, q);
      // printf("test: %s\n", tokens[p].str);
      switch (tokens[p].str[2]) {
        case 'a':
          return cpu.eax;
        case 'b':
          if (tokens[p].str[3] == 'x')
            return cpu.ebx;
          else if (tokens[p].str[3] == 'p')
            return cpu.ebp;
          else {
            *success = false;
            return -1;
          }
        case 'c':
          return cpu.ecx;
        case 'd':
          if (tokens[p].str[3] == 'x')
            return cpu.edx;
          else if (tokens[p].str[3] == 'i')
            return cpu.edi;
          else {
            *success = false;
            return -1;
          }
        case 's':          
          if (tokens[p].str[3] == 'p')
            return cpu.esp;
          else if (tokens[p].str[3] == 'i')
            return cpu.esi;
          else {
            *success = false;
            return -1;
          }
        case 'i':
          return cpu.eip; 
        default:
          *success = false;
          return -1;
      }
    }
    else if (tokens[p].type == TK_ID) {//id
      //TODO:不会写。。。
      printf("test: p==q TK_ID\n");
      return 0;
    } 
  } 
  else if (check_parentheses(p, q, success)) {
    if (*success == false) {//括号不匹配，停止递归直接返回
      // printf("test: wrong bracket\n");
      return -1;
    }
    return eval(p+1, q-1, success);//去掉括号继续递归
  } 
  else {
    // printf("test: eval else\n");
    int opPos = findDominantOp(p, q);
    // printf("test: opPos %d opType %d", opPos, tokens[opPos].type);
    switch (tokens[opPos].type) {
      case TK_ADD:
        // printf("test: TK_ADD");
        return eval(p, opPos-1, success) + eval(opPos+1, q, success);
      case TK_SUB:
        return eval(p, opPos-1, success) - eval(opPos+1, q, success);
      case TK_MUL:
        return eval(p, opPos-1, success) * eval(opPos+1, q, success);
      case TK_DIV:
        return eval(p, opPos-1, success) / eval(opPos+1, q, success);
      case TK_G:
        return eval(p, opPos-1, success) > eval(opPos+1, q, success);
      case TK_L:
        return eval(p, opPos-1, success) < eval(opPos+1, q, success);
      case TK_GE:
        return eval(p, opPos-1, success) >= eval(opPos+1, q, success);
      case TK_LE:
        return eval(p, opPos-1, success) <= eval(opPos+1, q, success);
      case TK_EQ:
        return eval(p, opPos-1, success) == eval(opPos+1, q, success);
      case TK_NEQ:
        return eval(p, opPos-1, success) != eval(opPos+1, q, success);
      case TK_AND:
        return eval(p, opPos-1, success) && eval(opPos+1, q, success);
      case TK_OR:
        return eval(p, opPos-1, success) || eval(opPos+1, q, success);
      case TK_NOT:
        return !eval(opPos+1, q, success);
      case TK_MINUS:
        return -eval(opPos+1, q, success);
      case TK_DEREFERENCE:
        //@可能有误
        return *(int*)(eval(opPos+1, q, success));
      default:
        *success = false;
        assert("Wrong in 'expr.c' eval().");
        return -1;
    }
  }
  *success = false;
  return -1;//不想每次都看见warning，所以加两句没用的
}


uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  // printf("test: expr() make token down.\n");
  /* TODO: Insert codes to evaluate the expression. */
  for (int i = 0; i < nr_token; i++) { //判断单目运算符*-
    if (tokens[i].type == TK_MUL || tokens[i].type == TK_SUB) {
      if (i == 0 || 
          (tokens[i-1].type != TK_NUM && tokens[i-1].type != TK_REG && 
           tokens[i-1].type != TK_RBRACKET && tokens[i-1].type != TK_ID)) {
        if (tokens[i].type == TK_SUB) {
          tokens[i].type = TK_MINUS;
        }
        else {
          tokens[i].type = TK_DEREFERENCE;
        }
      }
    }
  }
  // printf("test: expr.c expr\n");
  return eval(0, nr_token - 1, success);
}
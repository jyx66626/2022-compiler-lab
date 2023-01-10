#include <stdio.h>

#define NRW        12     // 保留字的数目
#define TXMAX 500         // 标识符表长度
#define MAXNUMLEN  14     // 最大数字位数
#define NSYM 10           // 数组ssym和csym中的最大符号数
#define MAXIDLEN 10       // 标识符的长度

#define MAXADDRESS 32767  // 最大地址
#define MAXLEVEL 32       // 嵌套块的最大深度
#define CXMAX 500         // 代码数组的大小

#define MAXSYM     30     // maximum number of symbols

#define STACKSIZE 1000 // 最大存储

enum symtype
{
  SYM_NULL,
  SYM_IDENTIFIER, // id
  SYM_NUMBER,     // 数字
  SYM_PLUS,       // 加
  SYM_MINUS,      // 乘
  SYM_TIMES,      // 星号*
  SYM_SLASH,      // 斜杠/
  SYM_ODD,        // 取反符号
  SYM_EQU,        //=
  SYM_NEQ,        // <>
  SYM_LES,        //<
  SYM_LEQ,        //<=
  SYM_GTR,        //>
  SYM_GEQ,        //>=
  SYM_LPAREN,     // (
  SYM_RPAREN,     // )
  SYM_COMMA,      // 逗号
  SYM_SEMICOLON,  // ;
  SYM_PERIOD,     // 结束符"."
  SYM_BECOMES,    //:=
  SYM_BEGIN,
  SYM_END,
  SYM_IF,
  SYM_THEN,
  SYM_WHILE,     // while
  SYM_DO,        // do
  SYM_CALL,      // call
  SYM_CONST,     // 常量
  SYM_VAR,       // var
  SYM_PROCEDURE, // procedure
  SYM_PRINT      // print
};

enum idtype
{
	ID_CONSTANT, ID_VARIABLE, ID_PROCEDURE,ID_ARRAY
};

enum opcode
{
  LIT, /* 将常数置于栈顶 */
  OPR, /* 一组算术或逻辑运算指令 */
  LOD, /* 将变量值置于栈顶 */
  STO, /* 将栈顶的值赋与某变量 */
  CAL, /* 用于过程调用的指令 */
  INT, /* 在数据栈中分配存贮空间 */
  JMP, /* 用于if语句的条件或无条件控制转移指令 */
  JPC,  /* 用于while语句的条件或无条件控制转移指令 */
  PRT /*PRT 0 a，逆序打印栈顶a个元素的值*/
};

enum oprcode
{
	OPR_RET, OPR_NEG, OPR_ADD, OPR_MIN,
	OPR_MUL, OPR_DIV, OPR_ODD, OPR_EQU,
	OPR_NEQ, OPR_LES, OPR_LEQ, OPR_GTR,
	OPR_GEQ
};


typedef struct
{
	int f; // function code
	int l; // level
	int a; // displacement address
} instruction;

//////////////////////////////////////////////////////////////////////
char* err_msg[] =
{
/*  0 */    "",
/*  1 */    "Found ':=' when expecting '='.",
/*  2 */    "There must be a number to follow '='.",
/*  3 */    "There must be an '=' to follow the identifier.",
/*  4 */    "There must be an identifier to follow 'const', 'var', or 'procedure'.",
/*  5 */    "Missing ',' or ';'.",
/*  6 */    "Incorrect procedure name.",
/*  7 */    "Statement expected.",
/*  8 */    "Follow the statement is an incorrect symbol.",
/*  9 */    "'.' expected.",
/* 10 */    "';' expected.",
/* 11 */    "Undeclared identifier.",
/* 12 */    "Illegal assignment.",
/* 13 */    "':=' expected.",
/* 14 */    "There must be an identifier to follow the 'call'.",
/* 15 */    "A constant or variable can not be called.",
/* 16 */    "'then' expected.",
/* 17 */    "';' or 'end' expected.",
/* 18 */    "'do' expected.",
/* 19 */    "Incorrect symbol.",
/* 20 */    "Relative operators expected.",
/* 21 */    "Procedure identifier can not be in an expression.",
/* 22 */    "Missing ')'.",
/* 23 */    "The symbol can not be followed by a factor.",
/* 24 */    "The symbol can not be as the beginning of an expression.",
/* 25 */    "The number is too great.",
/* 26 */    "",
/* 27 */    "",
/* 28 */    "",
/* 29 */    "",
/* 30 */    "",
/* 31 */    "",
/* 32 */    "There are too many levels."
/* 33 */    "print error."
};

//////////////////////////////////////////////////////////////////////
char ch;         // 最后读取的字符
int  sym;        // 最后读取的标志
char id[MAXIDLEN + 1]; // 最后读取的标识符
int  num;        // 最后读取的数字
int  cc;         // 字符计数
int  ll;         // 行的长度
int  kk;
int  err;
int cx; // 下一条即将生成的目标指令的地址
int  level = 0;
int  tx = 0;
int count=0;
char line[80];

instruction code[CXMAX]; // 程序（目标代码）存贮称为 code

char* word[NRW + 1] =
{
	"", /* 占位符 */
	"begin", "call", "const", "do", "end","if",
	"odd", "procedure", "then", "var", "while","print"
};

int wsym[NRW + 1] =
{
	SYM_NULL, SYM_BEGIN, SYM_CALL, SYM_CONST, SYM_DO, SYM_END,
	SYM_IF, SYM_ODD, SYM_PROCEDURE, SYM_THEN, SYM_VAR, SYM_WHILE,SYM_PRINT
};

int ssym[NSYM + 1] =
{
	SYM_NULL, SYM_PLUS, SYM_MINUS, SYM_TIMES, SYM_SLASH,
	SYM_LPAREN, SYM_RPAREN, SYM_EQU, SYM_COMMA, SYM_PERIOD, SYM_SEMICOLON
};

char csym[NSYM + 1] =
{
	' ', '+', '-', '*', '/', '(', ')', '=', ',', '.', ';'
};

#define MAXINS   9
char* mnemonic[MAXINS] =
{
	"LIT", "OPR", "LOD", "STO", "CAL", "INT", "JMP", "JPC","PRT"
};

typedef struct
{
	char name[MAXIDLEN + 1];
	int  kind;
	int  value;
} comtab;

comtab table[TXMAX];

typedef struct
{
	char  name[MAXIDLEN + 1];
	int   kind;
	short level;
	short address;
} mask;

FILE* infile;

// EOF PL0.h

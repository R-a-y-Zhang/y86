#ifndef Y86EMUL_H
#define Y86EMUL_H

typedef enum {
    AOK,
    HLT,
    ADR,
    INS
} state;

typedef struct Program {
    int OF;
    int ZF;
    int SF;

    int registers[8];

    unsigned char* memBlock;

    int IP;

    int memBlock_size;

    state state;
} Program;

void printError(char* error);
void printHelp();

Program* createProgram();
void terminator(Program *E);

void PreProcessor(Program *E, char* tokens);
void textDirective(Program *E, char* tokens);
void load_string(Program *E, char* tokens);
void load_long(Program *E, char* tokens);
void load_bss(Program *E, char* tokens);
void load_byte(Program *E, char* tokens);

void read_file(char* fileName);
void EXECUTE(Program *E);

unsigned int hexToLittleEndian (unsigned int x);

typedef struct opcodes {
    int codes[26];
    void (*fn[26])(Program*);
} opcodes;
opcodes* opcoder();
void rrmovl(Program *E);
void irmovl(Program *E);
void rmmovl(Program *E);
void mrmovl(Program *E);
void addl(Program *E);
void subl(Program *E);
void andl(Program *E);
void xorl(Program *E);
void mull(Program *E);
void cmpl(Program *E);
void jmp(Program *E);
void jle(Program *E);
void jl(Program *E);
void je(Program *E);
void jne(Program *E);
void jge(Program *E);
void jg(Program *E);
void call(Program *E);
void ret(Program *E);
void pushl(Program *E);
void popl(Program *E);
void readb(Program *E);
void readl(Program *E);
void writeb(Program *E);
void writel(Program *E);
void movsbl(Program *E);

#endif

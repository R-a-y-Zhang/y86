#include "y86emul.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int debug = 1;

// TODO put in multithreading
// TODO change how flags are set and got
// TODO move instruction definitions to separate file
//
void printError(char* error) {
    char* redColor = "\x1b[31m";
    char* resetColor = "\x1b[0m";
    fprintf(stderr, "%s%s%s\n", redColor, error, resetColor);
}

void printHelp() {
    printf("Welcome to the Y86 emulator. The basic operation of the emulator is:\n./y86emul <filename>.\nThe filename must have the extension .y86 and can contain a single .size and .text directive.\n");
}

Program* createProgram() {
    Program *prg = (Program*) malloc(sizeof(Program));
    prg->OF = 0;
    prg->ZF = 0;
    prg->SF = 0;

    int i;
    for (i = 0; i < 8; i++) {
        prg->registers[i] = 0;
    }

    prg->IP = 0;
    prg->memBlock_size = 0;
    prg->memBlock = NULL;
    prg->state = AOK;

    return prg;
}

void terminator(Program *prg) {
    free(prg->memBlock);
    free(prg);
}

void Allocator(Program *prg, char* tokens) {
    strtok(tokens, "\t");
    int size = strtol(strtok(NULL, "\t"), NULL, 16);
    prg->memBlock_size = size;
    prg->registers[4] = prg->memBlock_size - 1;
    prg->memBlock = (unsigned char*) malloc(size);
    int i;
    for (i = 0; i < size; i++) {
        prg->memBlock[i] = ' ';
    }
    prg->memBlock[size] = '\0';
}

void PreProcessor(Program *prg, char* tokens) {
    strtok(tokens, " \n\t");
    prg->IP = strtol(strtok(NULL, " \n\t"), NULL, 16);
    int currentIndex = prg->IP;

    char* instructions = strtok(NULL, " \n\t");
    int i;
    for (i = 0; i < strlen(instructions) - 2; i += 2) {
        char* currentByte = (char*) malloc(3);
        strncpy(currentByte, instructions + i, 2);
        currentByte[2] = '\0';
        unsigned char c = (unsigned char) strtol(currentByte, NULL, 16);
        prg->memBlock[currentIndex] = c;
        free(currentByte);
        currentIndex++;
    }
}

void load_byte(Program *prg, char* tokens) {
    strtok(tokens, " \n\t");
    int currentIndex = strtol(strtok(NULL, " \n\t"), NULL, 16);
    unsigned char c = (unsigned char) strtol(strtok(NULL, " \n\t"), NULL, 16);
    prg->memBlock[currentIndex] = c;
}

void load_string(Program *prg, char* tokens) {
    strtok(tokens, " \n\t");
    int currentIndex = strtol(strtok(NULL, " \n\t"), NULL, 16);
    char* string = strtok(NULL, " \n\t");
    int i;
    for (i = 1; i < strlen(string) - 2; i++) {
        prg->memBlock[currentIndex] = string[i];
        currentIndex++;
    }
}

void load_long(Program *prg, char* tokens) {
    strtok(tokens, " \n\t");
    int currentIndex = strtol(strtok(NULL, " \n\t"), NULL, 16);
    int number = strtol(strtok(NULL, " \n\t"), NULL, 10);
    unsigned int byte = 0;
    unsigned char c;
    int i;
    for (i = 0; i < 4; i++) {
        byte = (number >> (8*i)) & 0xff;
        c = byte;
        prg->memBlock[currentIndex] = c;
        currentIndex++;
    }
}

void load_bss(Program *prg, char* tokens) {
    strtok(tokens, " \n\t");
    int currentIndex = strtol(strtok(NULL, " \n\t"), NULL, 16);
    int number = strtol(strtok(NULL, " \n\t"), NULL, 10);
    int i;
    for (i = 0; i < number; i++) {
        prg->memBlock[currentIndex] = '\0';
        currentIndex++;
    }
}

void read_file(char* filepath) {
    FILE* fp;
    char* line = NULL;
    char* token = NULL;
    char* tokenizerInput = NULL;
    size_t len = 0;
    ssize_t read;

    int size_directive_counter = 0;
    int text_directive_count = 0;
    char* sizeLine = NULL;
    char* textLine = NULL;

    Program *prg = (Program*) malloc(sizeof(Program));

    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) {
        printError("error: Invalid filename.");
        exit(1);
    } else if (strcmp(dot + 1, "y86") != 0) {
        printError("error: Invalid extension.");
        exit(1);
    }

    fp = fopen(filepath, "r");

    if (fp == NULL) {
        printError("error: Unable to access file.");
        exit(1);
    }

    while ((read = getline(&line, &len, fp)) != -1) {
        tokenizerInput = strdup(line);
        token = strtok(tokenizerInput, " \n\t");
        if (strcmp(token, ".size") == 0) {
            size_directive_counter++;
            sizeLine = strdup(line);
        } else if (strcmp(token, ".text") == 0) {
            text_directive_count++;
            textLine = strdup(line);
        }
        token = NULL;
        free(tokenizerInput);
        tokenizerInput = NULL;
    }

    fclose(fp);

    if (size_directive_counter != 1 || text_directive_count != 1) {
        if (line) {
            free(line);
        }
        printError("error: Must only contain one .text and .size directive.");
        exit(1);
    }

    prg = createProgram();

    Allocator(prg, sizeLine);
    PreProcessor(prg, textLine);

    free(sizeLine);
    free(textLine);

    fp = fopen(filepath, "r");

    while ((read = getline(&line, &len, fp)) != -1) {
        tokenizerInput = strdup(line);
        token = strtok(tokenizerInput, " \n\t");
        if (strcmp(token, ".string") == 0) {
            load_string(prg, line);
        } else if (strcmp(token, ".long") == 0) {
            load_long(prg, line);
        } else if (strcmp(token, ".bss") == 0) {
            load_bss(prg, line);
        } else if (strcmp(token, ".byte") == 0) {
            load_byte(prg, line);
        }
        token = NULL;
        free(tokenizerInput);
        tokenizerInput = NULL;
    }

    fclose(fp);
    if (line) {
        free(line);
    }

    EXECUTE(prg);

    terminator(prg);
}

unsigned int hex_to_LE (unsigned int x) {
    return (x >> 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | (x << 24);
}

int check_register_bounds(int x) {
    return (x > -1) && (x < 8);
}

opcodes* opcoder() {
    opcodes* op = (opcodes*)malloc(sizeof(opcodes));
    op->codes[0] = 0x20; op->fn[0] = rrmovl;
    op->codes[1] = 0x30; op->fn[1] = irmovl;
    op->codes[2] = 0x40; op->fn[2] = rmmovl;
    op->codes[3] = 0x50; op->fn[3] = mrmovl;
    op->codes[4] = 0x60; op->fn[4] = addl;
    op->codes[5] = 0x61; op->fn[5] = subl;
    op->codes[6] = 0x62; op->fn[6] = andl;
    op->codes[7] = 0x63; op->fn[7] = xorl;
    op->codes[8] = 0x64; op->fn[8] = mull;
    op->codes[9] = 0x65; op->fn[9] = cmpl;
    op->codes[10] = 0x70; op->fn[10] = jmp;
    op->codes[11] = 0x71; op->fn[11] = jle;
    op->codes[12] = 0x72; op->fn[12] = jl;
    op->codes[13] = 0x73; op->fn[13] = je;
    op->codes[14] = 0x74; op->fn[14] = jne;
    op->codes[15] = 0x75; op->fn[15] = jge;
    op->codes[16] = 0x76; op->fn[16] = jg;
    op->codes[17] = 0x80; op->fn[17] = call;
    op->codes[18] = 0x90; op->fn[18] = ret;
    op->codes[19] = 0xa0; op->fn[19] = pushl;
    op->codes[20] = 0xb0; op->fn[20] = popl;
    op->codes[21] = 0xc0; op->fn[21] = readb;
    op->codes[22] = 0xc1; op->fn[22] = readl;
    op->codes[23] = 0xd0; op->fn[23] = writeb;
    op->codes[24] = 0xd1; op->fn[24] = writel;
    op->codes[25] = 0xe0; op->fn[25] = movsbl;
    return op;
}

void EXECUTE(Program *prg) {
    unsigned int x;
    opcodes* op = opcoder();
    while (prg->state == AOK) {
        x = prg->memBlock[prg->IP];
        prg->IP++;
        if (x == 0x00) {
            continue;
        } else if (x == 0x10) {
            prg->state = HLT;
        } else {
            int i;
            for (i = 0; i < 26; i++) {
                if (op->codes[i] == x) {
                    op->fn[i](prg);
                    break;
                }
            }
            if (i == 26) {
                prg->state = INS;
            }
        }

        if (prg->state == AOK) {
            prg->IP++;
        }
    }

    switch (prg->state) {
        case ADR:
            printError("\nerror: (adr) Invalid address encountered.");
            exit(1);
        break;

        case INS:
            printError("\nerror: (ins) Invalid instruction encountered.");
            exit(1);
        break;

        case HLT:
            printf("\n(hlt) Program terminated.\n");
            exit(1);
        break;

        default:
        break;
    }
}

void rrmovl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }

    prg->registers[regB] = prg->registers[regA];
}

void irmovl(Program *prg) {
    int reg = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(reg)) {
        prg->state = ADR;
        return;
    }

    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);
    prg->registers[reg] = value;
}

void rmmovl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }

    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);

    for (i = 0; i < 4; i++) {
        prg->memBlock[value + prg->registers[regB] + i] = (prg->registers[regA] >> (8*i)) & 0xff;
    }
}

void mrmovl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }

    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);

    int result = 0;
    for (i = 0; i < 4; i++) {
        result += (prg->memBlock[prg->registers[regB] + value + i]) << (8*i);
    }
    prg->registers[regA] = result;
}

void addl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }
    int v2 = prg->registers[regB];
    int v1 = prg->registers[regA];
    int sum = v2 + v1;
    if (sum < 0) {
        prg->SF = 1;
    } else if (sum == 0) {
        prg->ZF = 1;
    } else {
        prg->ZF = 0;
    }

    if ((v1 > 0 && v2 > 0 && sum < 0) || (v1 < 0 && v2 < 0 && sum > 0)) {
        prg->OF = 1;
    }

    prg->registers[regB] = sum;
}

void subl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }
    int v2 = prg->registers[regB];
    int v1 = prg->registers[regA];
    int difference = v2 - v1;
    if (difference < 0) {
        prg->SF = 1;
    } else if (difference == 0) {
        prg->ZF = 1;
    } else {
        prg->ZF = 0;
    }

    if ((difference < 0 && v2 < 0 && v1 > 0) || (difference > 0 && v2 > 0 && v1 < 0)) {
        prg->OF = 1;
    }

    prg->registers[regB] = difference;
}

void andl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }
    int v2 = prg->registers[regB];
    int v1 = prg->registers[regA];
    int result = v2 && v1;
    if (result < 0) {
        prg->SF = 1;
    } else if (result == 0) {
        prg->ZF = 1;
    }

    prg->registers[regB] = result;
}

void xorl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }
    int v2 = prg->registers[regB];
    int v1 = prg->registers[regA];
    int result = v2 ^ v1;
    if (result < 0) {
        prg->SF = 1;
    } else if (result == 0) {
        prg->ZF = 1;
    }
    prg->registers[regB] = result;
}

void mull(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }
    int v2 = (int) prg->registers[regB];
    int v1 = (int) prg->registers[regA];
    int product = v2 * v1;

    if (product < 0) {
        prg->SF = 1;
    } else if (product == 0) {
        prg->ZF = 1;
    }

    prg->registers[regB] = product;
}

void cmpl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    if (!check_register_bounds(regA) || !check_register_bounds(regB)) {
        prg->state = ADR;
        return;
    }
    int v2 = prg->registers[regB];
    int v1 = prg->registers[regA];
    int difference = v2 - v1;
    if (difference < 0) {
        prg->SF = 1;
    } else if (difference == 0) {
        prg->ZF = 1;
    }

    if ((difference < 0 && v2 < 0 && v1 > 0) || (difference > 0 && v2 > 0 && v1 < 0)) {
        prg->OF = 1;
    }
}

void jmp(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    prg->IP = value - 1;
}

void jle(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    if ((prg->SF ^ prg->OF) | prg->ZF) {
        prg->IP = value - 1;
    } else {
        prg->IP--;
    }
}

void jl(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    if (prg->SF ^ prg->OF) {
        prg->IP = value - 1;
    } else {
        prg->IP--;
    }
}

void je(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);

    if (prg->ZF) {
        prg->IP = value - 1;
    } else {
        prg->IP--;
    }
}

void jne(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    if (!prg->ZF) {
        prg->IP = value - 1;
    } else {
        prg->IP--;
    }
}

void jge(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    if (!(prg->SF ^ prg->OF)) {
        prg->IP = value - 1;
    } else {
        prg->IP--;
    }
}

void jg(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    if (!(prg->SF ^ prg->OF) && !prg->ZF) {
        prg->IP = value - 1;
    } else {
        prg->IP--;
    }
}

void call(Program *prg) {
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        value = (value << 8) | prg->memBlock[prg->IP];
        prg->IP++;
    }
    value = hex_to_LE(value);
    int savedIP = prg->IP;
    prg->IP = value - 1;

    prg->registers[4] -= 4;

    for (i = 0; i < 4; i++) {
        prg->memBlock[prg->registers[4] + i] = (savedIP >> (8*i)) & 0xff;
    }
}

void ret(Program *prg) {
    int savedIP = 0;
    int i;

    for (i = 0; i < 4; i++) {
        savedIP += (prg->memBlock[prg->registers[4] + i]) << (8*i);
    }

    prg->IP = savedIP - 1;
    prg->registers[4] += 4;
}

void pushl(Program *prg) {
    int reg = prg->memBlock[prg->IP] >> 4;
    int i;
    prg->registers[4] -= 4;
    for (i = 0; i < 4; i++) {
        prg->memBlock[prg->registers[4] + i] = (prg->registers[reg] >> (8*i)) & 0xff;
    }
}

void popl(Program *prg) {
    int reg = prg->memBlock[prg->IP] >> 4;
    int value = 0;
    int i;

    for (i = 0; i < 4; i++) {
        value += (prg->memBlock[prg->registers[4] + i]) << (8*i);
    }

    prg->registers[reg] = value;
    prg->registers[4] += 4;
}

void readb(Program *prg) {
    int reg = prg->memBlock[prg->IP] >> 4;
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);
    char c = getchar();
    prg->ZF = 0;
    prg->memBlock[value + prg->registers[reg]] = c;
}

void readl(Program *prg) {
    int reg = prg->memBlock[prg->IP] >> 4;
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);

    int num;
    
    int isEOF = scanf("%d", &num);

    if (num < 0 || isEOF == -1) {
        prg->ZF = 1;
        prg->state = HLT;
    } else {
        prg->ZF = 0;
    }

    for (i = 0; i < 4; i++) {
        prg->memBlock[value + prg->registers[reg] + i] = (num >> (8*i)) & 0xff;
    }
}

void writeb(Program *prg) {
    int reg = prg->memBlock[prg->IP] >> 4;
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);
    printf("%c", prg->memBlock[value + prg->registers[reg]]);
}

void writel(Program *prg) {
    int reg = prg->memBlock[prg->IP] >> 4;
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);

    int result = 0;

    for (i = 0; i < 4; i++) {
        result += (prg->memBlock[prg->registers[reg] + value + i]) << (8*i);
    }

    printf("%d", result);
}

void movsbl(Program *prg) {
    int regA = prg->memBlock[prg->IP] >> 4;
    int regB = prg->memBlock[prg->IP] & 0xf;
    int value;
    int i;
    for (i = 0; i < 4; i++) {
        prg->IP++;
        value = (value << 8) | prg->memBlock[prg->IP];
    }
    value = hex_to_LE(value);
    unsigned int n = prg->memBlock[value + prg->registers[regB]];
    n = (n << 24) >> 24;
    prg->registers[regA] = n;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        printError("error: You did not provide enough arguments!");
        exit(1);
    }

    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            printHelp();
            exit(0);
        }
    }

    read_file(argv[1]);

    return 0;
}

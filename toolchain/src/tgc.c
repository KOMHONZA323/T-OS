#include <stdio.h>
#include <string.h>

typedef enum { LANG_C, LANG_HOLYC, LANG_ASM } Language;

void compile(const char* file, Language lang) {
    printf("Compiling %s as %s...
", file, 
           lang == LANG_C ? "Standard C" : (lang == LANG_HOLYC ? "HolyC" : "Assembly"));
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: tgc <source_file> [-H]
");
        return 1;
    }

    const char* input = argv[1];
    Language lang = LANG_C;

    if (strstr(input, ".hc")) lang = LANG_HOLYC;
    if (strstr(input, ".asm") || strstr(input, ".s")) lang = LANG_ASM;
    
    // Override with -H
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-H") == 0) lang = LANG_HOLYC;
    }

    compile(input, lang);
    return 0;
}

#include <stdio.h>
#include <ctype.h>

typedef enum { TOKEN_INT, LANG_RETURN, TOKEN_ID, TOKEN_NUM, TOKEN_EOF } TokenType;

typedef struct {
    TokenType type;
    char value[64];
} Token;

void get_next_token(const char* input, int* pos, Token* tok) {
    while (isspace(input[*pos])) (*pos)++;
    
    if (isdigit(input[*pos])) {
        tok->type = TOKEN_NUM;
        // Collect number...
    } else if (isalpha(input[*pos])) {
        tok->type = TOKEN_ID;
        // Collect identifier...
    }
}

int main() {
    // T-OS C Compiler Frontend
    return 0;
}

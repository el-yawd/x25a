// parser.c
// Recursive-descent parser for X25a reading tokens produced by lexer.c
// Usage: ./parser tokens.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    T_EOF,
    T_KW_LEIA, T_KW_ESCREVA, T_KW_SE, T_KW_ENTAO, T_KW_SENAO, T_KW_FIM, T_KW_FACA, T_KW_ENQUANTO,
    T_ID, T_NUM,
    T_ASSIGN, T_LT, T_EQ, T_PLUS, T_MINUS, T_TIMES, T_DIV, T_COMMA, T_LPAREN, T_RPAREN,
    T_STRING,
    T_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[256];
} Token;

FILE* infile;
Token curtok;
int error_count = 0;     // Track errors found
int warning_count = 0;  // Track warnings separately

TokenType str_to_ttype(const char* s){
    if(strcmp(s,"EOF")==0) return T_EOF;
    if(strcmp(s,"KW_LEIA")==0) return T_KW_LEIA;
    if(strcmp(s,"KW_ESCREVA")==0) return T_KW_ESCREVA;
    if(strcmp(s,"KW_SE")==0) return T_KW_SE;
    if(strcmp(s,"KW_ENTAO")==0) return T_KW_ENTAO;
    if(strcmp(s,"KW_SENAO")==0) return T_KW_SENAO;
    if(strcmp(s,"KW_FIM")==0) return T_KW_FIM;
    if(strcmp(s,"KW_FACA")==0) return T_KW_FACA;
    if(strcmp(s,"KW_ENQUANTO")==0) return T_KW_ENQUANTO;
    if(strcmp(s,"ID")==0) return T_ID;
    if(strcmp(s,"NUM")==0) return T_NUM;
    if(strcmp(s,"ASSIGN")==0) return T_ASSIGN;
    if(strcmp(s,"LT")==0) return T_LT;
    if(strcmp(s,"EQ")==0) return T_EQ;
    if(strcmp(s,"PLUS")==0) return T_PLUS;
    if(strcmp(s,"MINUS")==0) return T_MINUS;
    if(strcmp(s,"TIMES")==0) return T_TIMES;
    if(strcmp(s,"DIV")==0) return T_DIV;
    if(strcmp(s,"COMMA")==0) return T_COMMA;
    if(strcmp(s,"LPAREN")==0) return T_LPAREN;
    if(strcmp(s,"RPAREN")==0) return T_RPAREN;
    if(strcmp(s,"STRING")==0) return T_STRING;
    return T_ERROR;
}

int read_token(){
    char tokname[64];
    char lexeme[256];
    if(fscanf(infile, " %63s", tokname) != 1) return 0;
    // if token has lexeme after tab/spaces, read rest of line
    int c = fgetc(infile);
    if(c == '\t' || c == ' '){
        // read rest of token lexeme up to newline
        if(fgets(lexeme, sizeof(lexeme), infile) == NULL) lexeme[0] = 0;
        else {
            // strip newline and leading spaces/tabs
            char* p = lexeme;
            while(*p == ' ' || *p == '\t') p++;
            char* nl = strchr(p, '\n');
            if(nl) *nl = 0;
            memmove(lexeme, p, strlen(p)+1);
        }
    } else {
        // no lexeme; push back the char
        ungetc(c, infile);
        lexeme[0] = 0;
    }
    curtok.type = str_to_ttype(tokname);
    strncpy(curtok.lexeme, lexeme, sizeof(curtok.lexeme)-1);
    curtok.lexeme[sizeof(curtok.lexeme)-1] = 0;
    return 1;
}

// Get human-readable token type name
const char* token_type_name(TokenType t) {
    switch(t) {
        case T_EOF: return "EOF";
        case T_KW_LEIA: return "LEIA";
        case T_KW_ESCREVA: return "ESCREVA";
        case T_KW_SE: return "SE";
        case T_KW_ENTAO: return "ENTÃO";
        case T_KW_SENAO: return "SENÃO";
        case T_KW_FIM: return "FIM";
        case T_KW_FACA: return "FAÇA";
        case T_KW_ENQUANTO: return "ENQUANTO";
        case T_ID: return "identifier";
        case T_NUM: return "number";
        case T_ASSIGN: return ":=";
        case T_LT: return "<";
        case T_EQ: return "=";
        case T_PLUS: return "+";
        case T_MINUS: return "-";
        case T_TIMES: return "*";
        case T_DIV: return "/";
        case T_COMMA: return ",";
        case T_LPAREN: return "(";
        case T_RPAREN: return ")";
        case T_STRING: return "string";
        default: return "UNKNOWN";
    }
}

void syntax_error(const char* msg){
    error_count++;
    fprintf(stderr, "\n[Error %d]: %s\n", error_count, msg);
    if(curtok.type == T_EOF) {
        fprintf(stderr, "  Found: EOF\n");
    } else {
        fprintf(stderr, "  Found: %s", token_type_name(curtok.type));
        if(strlen(curtok.lexeme) > 0) {
            fprintf(stderr, " '%s'", curtok.lexeme);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}

void syntax_warning(const char* msg){
    warning_count++;
    fprintf(stderr, "\n[Warning %d]: %s\n", warning_count, msg);
    if(curtok.type == T_EOF) {
        fprintf(stderr, "  Found: EOF\n");
    } else {
        fprintf(stderr, "  Found: %s", token_type_name(curtok.type));
        if(strlen(curtok.lexeme) > 0) {
            fprintf(stderr, " '%s'", curtok.lexeme);
        }
        fprintf(stderr, "\n");
    }
}

// Panic mode recovery: skip tokens until synchronization point
void panic_mode_recovery() {
    fprintf(stderr, "  Recovery: Skipping to next synchronization point...\n\n");
    
    // Synchronization tokens: comma, structural keywords, EOF
    while(curtok.type != T_EOF) {
        if(curtok.type == T_COMMA || 
           curtok.type == T_KW_FIM || 
           curtok.type == T_KW_SENAO || 
           curtok.type == T_KW_ENQUANTO) {
            // Found sync point
            if(curtok.type == T_COMMA) {
                read_token();  // consume comma and continue
            }
            break;
        }
        if(!read_token()) break;  // consume token
    }
}

void expect(TokenType t){
    if(curtok.type == t){
        if(!read_token()){ /* reached EOF unexpectedly */ }
    } else {
        char buf[256];
        snprintf(buf, sizeof(buf), "Expected %s", token_type_name(t));
        syntax_error(buf);
        panic_mode_recovery();  // Try to recover
    }
}

/* Forward declarations for recursive-descent functions */
void parse_PROGRAM();
void parse_DECL_LIST();
void parse_REST_DECLS();
void parse_DECL();
void parse_ASSIGNMENT();
void parse_READ_ST();
void parse_WRITE_ST();
void parse_IF_ST();
void parse_DO_WHILE_ST();
void parse_REL_EXPR();
void parse_EXPR();
void parse_EXPR_PRIME();
void parse_TERM();
void parse_TERM_PRIME();
void parse_FACTOR();

/* Check if current token is a block terminator */
int is_block_end(){
    return curtok.type == T_KW_SENAO ||
           curtok.type == T_KW_FIM ||
           curtok.type == T_KW_ENQUANTO ||
           curtok.type == T_EOF;
}

void parse_PROGRAM(){
    // DECL_LIST EOF
    parse_DECL_LIST();
    
    // Generic error recovery: consume any unexpected tokens before EOF
    while(curtok.type != T_EOF) {
        warning_count++;
        syntax_warning("Unexpected token at end of program");
        
        // Provide helpful hint based on token type
        if(curtok.type == T_KW_FIM) {
            fprintf(stderr, "  Hint: FIM is only used to close SE...ENTÃO blocks\n");
        } else if(curtok.type == T_KW_SENAO) {
            fprintf(stderr, "  Hint: SENÃO must be part of a SE...ENTÃO...SENÃO...FIM block\n");
        } else if(curtok.type == T_KW_ENQUANTO) {
            fprintf(stderr, "  Hint: ENQUANTO must be part of a FAÇA...ENQUANTO loop\n");
        } else {
            fprintf(stderr, "  Hint: Expected end of file after all declarations\n");
        }
        
        fprintf(stderr, "  Recovery: Ignoring this token and continuing...\n");
        
        // Consume the unexpected token (panic mode recovery)
        if(!read_token()) break;  // safety: avoid infinite loop
    }
    
    // Report final results
    printf("\n");
    if(error_count == 0 && warning_count == 0) {
        printf("✓ Parse successful: input is syntactically correct.\n");
    } else if(error_count == 0) {
        printf("⚠ Parse completed with %d warning(s) - syntax is acceptable.\n", warning_count);
    } else {
        printf("✗ Parse completed with %d error(s) and %d warning(s).\n", error_count, warning_count);
    }
}

void parse_DECL_LIST(){
    // DECL REST_DECLS
    parse_DECL();
    parse_REST_DECLS();
}

void parse_REST_DECLS(){
    // Consume comma if present (but it's optional before block end)
    if(curtok.type == T_COMMA){
        expect(T_COMMA);
        // Check if we're at a block terminator after comma
        if(is_block_end()){
            // Trailing comma before block end - allow it but don't continue
            return;
        }
        parse_DECL();
        parse_REST_DECLS();
    } else {
        // epsilon - no more declarations
    }
}

void parse_DECL(){
    switch(curtok.type){
        case T_ID: parse_ASSIGNMENT(); break;
        case T_KW_LEIA: parse_READ_ST(); break;
        case T_KW_ESCREVA: parse_WRITE_ST(); break;
        case T_KW_SE: parse_IF_ST(); break;
        case T_KW_FACA: parse_DO_WHILE_ST(); break;
        default: 
            syntax_error("Expected declaration: assignment (ID), LEIA, ESCREVA, SE, or FAÇA");
            panic_mode_recovery();
            break;
    }
}

void parse_ASSIGNMENT(){
    // ID ASSIGN EXPR
    expect(T_ID);
    expect(T_ASSIGN);
    parse_EXPR();
}

void parse_READ_ST(){
    expect(T_KW_LEIA);
    expect(T_ID);
}

void parse_WRITE_ST(){
    expect(T_KW_ESCREVA);
    if(curtok.type == T_ID) {
        expect(T_ID);
    } else if(curtok.type == T_STRING) {
        expect(T_STRING);
    } else {
        syntax_error("ESCREVA expects an identifier or string literal");
        panic_mode_recovery();
    }
}

void parse_IF_ST(){
    // KW_SE REL_EXPR KW_ENTAO DECL_LIST [KW_SENAO DECL_LIST] KW_FIM
    expect(T_KW_SE);
    parse_REL_EXPR();
    expect(T_KW_ENTAO);
    parse_DECL_LIST();
    if(curtok.type == T_KW_SENAO){
        expect(T_KW_SENAO);
        parse_DECL_LIST();
    }
    expect(T_KW_FIM);
}

void parse_DO_WHILE_ST(){
    // KW_FACA DECL_LIST KW_ENQUANTO REL_EXPR
    expect(T_KW_FACA);
    parse_DECL_LIST();
    expect(T_KW_ENQUANTO);
    parse_REL_EXPR();
}

void parse_REL_EXPR(){
    // EXPR REL_OP EXPR
    parse_EXPR();
    if(curtok.type == T_LT || curtok.type == T_EQ){
        expect(curtok.type);
    } else {
        syntax_error("Expected relational operator: '<' (less than) or '=' (equals)");
        panic_mode_recovery();
        return;
    }
    parse_EXPR();
}

void parse_EXPR(){
    parse_TERM();
    parse_EXPR_PRIME();
}

void parse_EXPR_PRIME(){
    if(curtok.type == T_PLUS){
        expect(T_PLUS);
        parse_TERM();
        parse_EXPR_PRIME();
    } else if(curtok.type == T_MINUS){
        expect(T_MINUS);
        parse_TERM();
        parse_EXPR_PRIME();
    } else {
        // epsilon
    }
}

void parse_TERM(){
    parse_FACTOR();
    parse_TERM_PRIME();
}

void parse_TERM_PRIME(){
    if(curtok.type == T_TIMES){
        expect(T_TIMES);
        parse_FACTOR();
        parse_TERM_PRIME();
    } else if(curtok.type == T_DIV){
        expect(T_DIV);
        parse_FACTOR();
        parse_TERM_PRIME();
    } else {
        // epsilon
    }
}

void parse_FACTOR(){
    if(curtok.type == T_NUM) { expect(T_NUM); return; }
    if(curtok.type == T_ID) { expect(T_ID); return; }
    if(curtok.type == T_LPAREN){
        expect(T_LPAREN);
        parse_EXPR();
        expect(T_RPAREN);
        return;
    }
    syntax_error("Expected expression factor: number, identifier, or '(' expression ')'");
    panic_mode_recovery();
}

int main(int argc, char** argv){
    if(argc < 2){ 
        fprintf(stderr, "Usage: %s tokens.txt\n", argv[0]); 
        fprintf(stderr, "  tokens.txt: file containing tokens generated by lexer\n");
        return 1; 
    }
    
    infile = fopen(argv[1], "r");
    if(!infile){ 
        fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
        perror("fopen"); 
        return 1; 
    }

    if(!read_token()){ 
        fprintf(stderr, "Error: No tokens found in input file\n"); 
        fclose(infile);
        return 1; 
    }
    
    printf("=== Starting Syntax Analysis ===\n\n");
    parse_PROGRAM();
    fclose(infile);
    
    printf("\n=== Summary ===\n");
    printf("Errors: %d\n", error_count);
    printf("Warnings: %d\n", warning_count);
    printf("\n");
    
    // Return 0 for success, 1 for errors (warnings are OK)
    return (error_count > 0) ? 1 : 0;
}

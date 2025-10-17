// parser.c
// LL(1) Recursive-descent parser for X25a with comprehensive error recovery
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
    int line_number;  // Track position for better error messages
} Token;

FILE* infile;
Token curtok;
int error_count = 0;
int warning_count = 0;
int token_count = 0;  // Track tokens processed

// LL(1) FIRST and FOLLOW sets for intelligent recovery
typedef enum {
    SYNC_NONE = 0,
    SYNC_COMMA = 1 << 0,
    SYNC_FIM = 1 << 1,
    SYNC_SENAO = 1 << 2,
    SYNC_ENQUANTO = 1 << 3,
    SYNC_EOF = 1 << 4,
    SYNC_RPAREN = 1 << 5,
    SYNC_DECL_START = 1 << 6,  // ID, LEIA, ESCREVA, SE, FACA
    SYNC_PLUS = 1 << 7,
    SYNC_MINUS = 1 << 8,
    SYNC_TIMES = 1 << 9,
    SYNC_DIV = 1 << 10
} SyncSet;

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

    int c = fgetc(infile);
    if(c == '\t' || c == ' '){
        if(fgets(lexeme, sizeof(lexeme), infile) == NULL) {
            lexeme[0] = 0;
        } else {
            char* p = lexeme;
            while(*p == ' ' || *p == '\t') p++;
            char* nl = strchr(p, '\n');
            if(nl) *nl = 0;
            memmove(lexeme, p, strlen(p)+1);
        }
    } else {
        if(c != EOF) ungetc(c, infile);
        lexeme[0] = 0;
    }

    curtok.type = str_to_ttype(tokname);
    strncpy(curtok.lexeme, lexeme, sizeof(curtok.lexeme)-1);
    curtok.lexeme[sizeof(curtok.lexeme)-1] = 0;
    curtok.line_number = ++token_count;

    return 1;
}

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
        case T_STRING: return "string literal";
        default: return "UNKNOWN";
    }
}

void syntax_error(const char* msg){
    error_count++;
    fprintf(stderr, "\n╔════════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║ SYNTAX ERROR #%d (Token Position: %d)\n", error_count, curtok.line_number);
    fprintf(stderr, "╠════════════════════════════════════════════════════════════╣\n");
    fprintf(stderr, "║ %s\n", msg);
    fprintf(stderr, "║ Found: %s", token_type_name(curtok.type));
    if(strlen(curtok.lexeme) > 0) {
        fprintf(stderr, " '%s'", curtok.lexeme);
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "╚════════════════════════════════════════════════════════════╝\n");
}

// Check if current token is in the synchronization set
int in_sync_set(SyncSet sync) {
    if((sync & SYNC_COMMA) && curtok.type == T_COMMA) return 1;
    if((sync & SYNC_FIM) && curtok.type == T_KW_FIM) return 1;
    if((sync & SYNC_SENAO) && curtok.type == T_KW_SENAO) return 1;
    if((sync & SYNC_ENQUANTO) && curtok.type == T_KW_ENQUANTO) return 1;
    if((sync & SYNC_EOF) && curtok.type == T_EOF) return 1;
    if((sync & SYNC_RPAREN) && curtok.type == T_RPAREN) return 1;
    if((sync & SYNC_DECL_START) && (curtok.type == T_ID || curtok.type == T_KW_LEIA ||
                                     curtok.type == T_KW_ESCREVA || curtok.type == T_KW_SE ||
                                     curtok.type == T_KW_FACA)) return 1;
    if((sync & SYNC_PLUS) && curtok.type == T_PLUS) return 1;
    if((sync & SYNC_MINUS) && curtok.type == T_MINUS) return 1;
    if((sync & SYNC_TIMES) && curtok.type == T_TIMES) return 1;
    if((sync & SYNC_DIV) && curtok.type == T_DIV) return 1;
    return 0;
}

// LL(1) panic mode recovery with synchronization set
void panic_mode_recovery(SyncSet sync) {
    fprintf(stderr, "  → Recovery Strategy: Skipping tokens until synchronization point\n");
    fprintf(stderr, "  → Looking for: ");

    int first = 1;
    if(sync & SYNC_COMMA) { fprintf(stderr, "%sCOMMA", first ? "" : ", "); first = 0; }
    if(sync & SYNC_FIM) { fprintf(stderr, "%sFIM", first ? "" : ", "); first = 0; }
    if(sync & SYNC_SENAO) { fprintf(stderr, "%sSENÃO", first ? "" : ", "); first = 0; }
    if(sync & SYNC_ENQUANTO) { fprintf(stderr, "%sENQUANTO", first ? "" : ", "); first = 0; }
    if(sync & SYNC_RPAREN) { fprintf(stderr, "%s)", first ? "" : ", "); first = 0; }
    if(sync & SYNC_DECL_START) { fprintf(stderr, "%sdeclaration start", first ? "" : ", "); first = 0; }
    if(sync & SYNC_EOF) { fprintf(stderr, "%sEOF", first ? "" : ", "); first = 0; }
    fprintf(stderr, "\n\n");

    int max_skip = 50;  // Prevent infinite loops
    int skipped = 0;

    while(!in_sync_set(sync) && curtok.type != T_EOF && skipped < max_skip) {
        fprintf(stderr, "  ... skipping %s", token_type_name(curtok.type));
        if(strlen(curtok.lexeme) > 0) {
            fprintf(stderr, " '%s'", curtok.lexeme);
        }
        fprintf(stderr, "\n");

        if(!read_token()) break;
        skipped++;
    }

    if(skipped >= max_skip) {
        fprintf(stderr, "  ✗ Recovery failed: too many tokens skipped\n\n");
    } else if(curtok.type != T_EOF) {
        fprintf(stderr, "  ✓ Recovery successful: found %s\n\n", token_type_name(curtok.type));
    }
}

// Enhanced expect with context-aware error messages
void expect(TokenType t, const char* context){
    if(curtok.type == t){
        read_token();
    } else {
        char buf[512];
        snprintf(buf, sizeof(buf), "Expected %s%s%s",
                 token_type_name(t),
                 context ? " in " : "",
                 context ? context : "");
        syntax_error(buf);

        // Provide helpful recovery hints
        if(t == T_ASSIGN && curtok.type == T_EQ) {
            fprintf(stderr, "  Hint: Use ':=' for assignment, not '='\n");
        } else if(t == T_KW_ENTAO && curtok.type == T_KW_FIM) {
            fprintf(stderr, "  Hint: SE requires ENTÃO before the body\n");
        } else if(t == T_KW_FIM && curtok.type == T_KW_ENQUANTO) {
            fprintf(stderr, "  Hint: This might be a FAÇA...ENQUANTO loop (no FIM needed)\n");
        } else if(t == T_KW_ENQUANTO && curtok.type == T_KW_FIM) {
            fprintf(stderr, "  Hint: FAÇA loops end with ENQUANTO condition, not FIM\n");
        }

        // Don't skip the token if it might be useful for recovery
        if(t == T_COMMA && in_sync_set(SYNC_DECL_START)) {
            // We're expecting comma but found declaration start - continue without comma
            return;
        }

        // Skip the erroneous token unless it's a synchronization point
        if(!in_sync_set(SYNC_COMMA | SYNC_FIM | SYNC_SENAO | SYNC_ENQUANTO | SYNC_EOF)) {
            read_token();
        }
    }
}

/* Forward declarations */
void parse_PROGRAM();
void parse_DECL_LIST(SyncSet follow);
void parse_REST_DECLS(SyncSet follow);
void parse_DECL(SyncSet follow);
void parse_ASSIGNMENT(SyncSet follow);
void parse_READ_ST(SyncSet follow);
void parse_WRITE_ST(SyncSet follow);
void parse_IF_ST(SyncSet follow);
void parse_DO_WHILE_ST(SyncSet follow);
void parse_REL_EXPR(SyncSet follow);
void parse_EXPR(SyncSet follow);
void parse_EXPR_PRIME(SyncSet follow);
void parse_TERM(SyncSet follow);
void parse_TERM_PRIME(SyncSet follow);
void parse_FACTOR(SyncSet follow);

// Check if token can start a declaration (FIRST set)
int is_decl_start(){
    return curtok.type == T_ID || curtok.type == T_KW_LEIA ||
           curtok.type == T_KW_ESCREVA || curtok.type == T_KW_SE ||
           curtok.type == T_KW_FACA;
}

void parse_PROGRAM(){
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Starting LL(1) Syntax Analysis\n");
    printf("═══════════════════════════════════════════════════════════\n\n");

    parse_DECL_LIST(SYNC_EOF);

    // Handle trailing tokens
    while(curtok.type != T_EOF) {
        syntax_error("Unexpected token after end of program");

        if(curtok.type == T_KW_FIM) {
            fprintf(stderr, "  Hint: Extra FIM - check if SE blocks are balanced\n");
        } else if(curtok.type == T_KW_SENAO) {
            fprintf(stderr, "  Hint: SENÃO without matching SE...ENTÃO\n");
        } else if(curtok.type == T_KW_ENQUANTO) {
            fprintf(stderr, "  Hint: ENQUANTO without matching FAÇA\n");
        }

        if(!read_token()) break;
    }

    // Final summary
    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  Analysis Complete\n");
    printf("═══════════════════════════════════════════════════════════\n");

    if(error_count == 0) {
        printf("\n✓ SUCCESS: Program is syntactically correct!\n");
        printf("  All %d tokens parsed successfully.\n", token_count);
    } else {
        printf("\n✗ FAILED: Found %d error(s)\n", error_count);
        printf("  Please fix the errors and try again.\n");
    }
}

void parse_DECL_LIST(SyncSet follow){
    // DECL_LIST → DECL REST_DECLS
    if(is_decl_start()) {
        parse_DECL(follow | SYNC_COMMA | SYNC_DECL_START);
        parse_REST_DECLS(follow);
    } else if(!in_sync_set(follow)) {
        syntax_error("Expected declaration (assignment, LEIA, ESCREVA, SE, or FAÇA)");
        panic_mode_recovery(follow | SYNC_DECL_START);
        if(is_decl_start()) {
            parse_DECL(follow | SYNC_COMMA | SYNC_DECL_START);
            parse_REST_DECLS(follow);
        }
    }
}


void parse_REST_DECLS(SyncSet follow) {
    if (curtok.type == T_COMMA) {
        expect(T_COMMA, "declaration separator");

        // ⬇️ Move this before trying to parse another declaration
        if (in_sync_set(follow)) {
            // trailing comma before FIM / SENAO / ENQUANTO / EOF is OK
            return;
        }

        if (is_decl_start()) {
            parse_DECL(follow | SYNC_COMMA | SYNC_DECL_START);
            parse_REST_DECLS(follow);
        } else if (!in_sync_set(follow)) {   // only complain if not end-of-block
            syntax_error("Expected declaration after comma");
            panic_mode_recovery(follow | SYNC_DECL_START);
            if (is_decl_start()) {
                parse_DECL(follow | SYNC_COMMA | SYNC_DECL_START);
                parse_REST_DECLS(follow);
            }
        }
    } else if (is_decl_start()) {
        parse_DECL(follow | SYNC_COMMA | SYNC_DECL_START);
        parse_REST_DECLS(follow);
    }
    // else: epsilon
}


void parse_DECL(SyncSet follow){
    switch(curtok.type){
        case T_ID:
            parse_ASSIGNMENT(follow);
            break;
        case T_KW_LEIA:
            parse_READ_ST(follow);
            break;
        case T_KW_ESCREVA:
            parse_WRITE_ST(follow);
            break;
        case T_KW_SE:
            parse_IF_ST(follow);
            break;
        case T_KW_FACA:
            parse_DO_WHILE_ST(follow);
            break;
        default:
            syntax_error("Invalid declaration start");
            panic_mode_recovery(follow);
            break;
    }
}

void parse_ASSIGNMENT(SyncSet follow){
    // ID ASSIGN EXPR
    expect(T_ID, "assignment");
    expect(T_ASSIGN, "assignment");
    parse_EXPR(follow);
}

void parse_READ_ST(SyncSet follow){
    // LEIA ID
    expect(T_KW_LEIA, "read statement");
    expect(T_ID, "LEIA statement (variable name required)");
}

void parse_WRITE_ST(SyncSet follow){
    // ESCREVA (ID | STRING)
    expect(T_KW_ESCREVA, "write statement");

    if(curtok.type == T_ID) {
        expect(T_ID, NULL);
    } else if(curtok.type == T_STRING) {
        expect(T_STRING, NULL);
    } else {
        syntax_error("ESCREVA requires identifier or string literal");
        panic_mode_recovery(follow);
    }
}

void parse_IF_ST(SyncSet follow){
    // SE REL_EXPR ENTÃO DECL_LIST [SENÃO DECL_LIST] FIM
    expect(T_KW_SE, "conditional statement");
    parse_REL_EXPR(SYNC_ENQUANTO);
    expect(T_KW_ENTAO, "SE statement (condition must be followed by ENTÃO)");
    parse_DECL_LIST(SYNC_SENAO | SYNC_FIM);

    if(curtok.type == T_KW_SENAO){
        expect(T_KW_SENAO, NULL);
        parse_DECL_LIST(SYNC_FIM);
    }

    expect(T_KW_FIM, "SE block (must close with FIM)");
}

void parse_DO_WHILE_ST(SyncSet follow){
    // FAÇA DECL_LIST ENQUANTO REL_EXPR
    expect(T_KW_FACA, "do-while loop");
    parse_DECL_LIST(SYNC_ENQUANTO);
    expect(T_KW_ENQUANTO, "FAÇA loop (must end with ENQUANTO condition)");
    parse_REL_EXPR(follow);
}

void parse_REL_EXPR(SyncSet follow){
    // EXPR REL_OP EXPR where REL_OP ∈ {<, =}
    parse_EXPR(SYNC_NONE);

    if(curtok.type == T_LT || curtok.type == T_EQ){
        TokenType op = curtok.type;
        expect(op, "relational expression");
    } else {
        syntax_error("Expected relational operator ('<' or '=')");
        fprintf(stderr, "  Hint: X25a only supports '<' (less than) and '=' (equals)\n");
        panic_mode_recovery(follow);
        return;
    }

    parse_EXPR(follow);
}

void parse_EXPR(SyncSet follow){
    // EXPR → TERM EXPR'
    parse_TERM(SYNC_PLUS | SYNC_MINUS | follow);
    parse_EXPR_PRIME(follow);
}

void parse_EXPR_PRIME(SyncSet follow){
    // EXPR' → + TERM EXPR' | - TERM EXPR' | ε
    if(curtok.type == T_PLUS){
        expect(T_PLUS, NULL);
        parse_TERM(SYNC_PLUS | SYNC_MINUS | follow);
        parse_EXPR_PRIME(follow);
    } else if(curtok.type == T_MINUS){
        expect(T_MINUS, NULL);
        parse_TERM(SYNC_PLUS | SYNC_MINUS | follow);
        parse_EXPR_PRIME(follow);
    }
    // else: epsilon
}

void parse_TERM(SyncSet follow){
    // TERM → FACTOR TERM'
    parse_FACTOR(SYNC_TIMES | SYNC_DIV | follow);
    parse_TERM_PRIME(follow);
}

void parse_TERM_PRIME(SyncSet follow){
    // TERM' → * FACTOR TERM' | / FACTOR TERM' | ε
    if(curtok.type == T_TIMES){
        expect(T_TIMES, NULL);
        parse_FACTOR(SYNC_TIMES | SYNC_DIV | follow);
        parse_TERM_PRIME(follow);
    } else if(curtok.type == T_DIV){
        expect(T_DIV, NULL);
        parse_FACTOR(SYNC_TIMES | SYNC_DIV | follow);
        parse_TERM_PRIME(follow);
    }
    // else: epsilon
}

void parse_FACTOR(SyncSet follow){
    // FACTOR → NUM | ID | ( EXPR )
    if(curtok.type == T_NUM) {
        expect(T_NUM, NULL);
    } else if(curtok.type == T_ID) {
        expect(T_ID, NULL);
    } else if(curtok.type == T_LPAREN){
        expect(T_LPAREN, "expression");
        parse_EXPR(SYNC_RPAREN);
        expect(T_RPAREN, "parenthesized expression");
    } else {
        syntax_error("Expected expression factor (number, identifier, or '(')");
        fprintf(stderr, "  Hint: Valid factors are numbers, variables, or (expression)\n");
        panic_mode_recovery(follow);
    }
}

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Usage: %s tokens.txt\n", argv[0]);
        fprintf(stderr, "  tokens.txt: token file generated by lexer\n");
        return 1;
    }

    infile = fopen(argv[1], "r");
    if(!infile){
        fprintf(stderr, "Error: Cannot open '%s'\n", argv[1]);
        perror("fopen");
        return 1;
    }

    if(!read_token()){
        fprintf(stderr, "Error: Empty token file\n");
        fclose(infile);
        return 1;
    }

    parse_PROGRAM();
    fclose(infile);

    printf("\n═══════════════════════════════════════════════════════════\n");
    printf("  Final Statistics\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("  Tokens Processed: %d\n", token_count);
    printf("  Errors Found:     %d\n", error_count);
    printf("  Warnings Issued:  %d\n", warning_count);
    printf("═══════════════════════════════════════════════════════════\n\n");

    return (error_count > 0) ? 1 : 0;
}

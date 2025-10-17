// lexer.c
// Simple DFA-based lexer for X25a with proper UTF-8 support
// Usage: ./lexer input.x25a > tokens.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Global error/warning counters
int error_count = 0;
int warning_count = 0;

typedef enum {
    T_EOF,
    T_KW_LEIA, T_KW_ESCREVA, T_KW_SE, T_KW_ENTAO, T_KW_SENAO, T_KW_FIM, T_KW_FACA, T_KW_ENQUANTO,
    T_ID, T_NUM,
    T_ASSIGN, T_LT, T_EQ, T_PLUS, T_MINUS, T_TIMES, T_DIV, T_COMMA, T_LPAREN, T_RPAREN,
    T_STRING,
    T_ERROR
} TokenType;

const char* token_name(TokenType t) {
    switch(t){
        case T_EOF: return "EOF";
        case T_KW_LEIA: return "KW_LEIA";
        case T_KW_ESCREVA: return "KW_ESCREVA";
        case T_KW_SE: return "KW_SE";
        case T_KW_ENTAO: return "KW_ENTAO";
        case T_KW_SENAO: return "KW_SENAO";
        case T_KW_FIM: return "KW_FIM";
        case T_KW_FACA: return "KW_FACA";
        case T_KW_ENQUANTO: return "KW_ENQUANTO";
        case T_ID: return "ID";
        case T_NUM: return "NUM";
        case T_ASSIGN: return "ASSIGN";
        case T_LT: return "LT";
        case T_EQ: return "EQ";
        case T_PLUS: return "PLUS";
        case T_MINUS: return "MINUS";
        case T_TIMES: return "TIMES";
        case T_DIV: return "DIV";
        case T_COMMA: return "COMMA";
        case T_LPAREN: return "LPAREN";
        case T_RPAREN: return "RPAREN";
        case T_STRING: return "STRING";
        default: return "ERROR";
    }
}

/* --- UTF-8 decoding --- */
int utf8_next(FILE* f, unsigned char* buf, int* len, unsigned int* codepoint) {
    int c = fgetc(f);
    if (c == EOF) return 0;

    buf[0] = (unsigned char)c;
    if (c < 0x80) { // ASCII
        *len = 1;
        *codepoint = c;
        return 1;
    }

    int needed = 0;
    if ((c & 0xE0) == 0xC0) needed = 2;
    else if ((c & 0xF0) == 0xE0) needed = 3;
    else if ((c & 0xF8) == 0xF0) needed = 4;
    else return -1; // invalid UTF-8 start

    buf[1] = buf[2] = buf[3] = 0;
    for (int i = 1; i < needed; i++) {
        int d = fgetc(f);
        if (d == EOF || (d & 0xC0) != 0x80)
            return -1;
        buf[i] = (unsigned char)d;
    }

    *len = needed;
    // reconstruct codepoint
    unsigned int cp = 0;
    if (needed == 2) cp = ((buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
    else if (needed == 3) cp = ((buf[0] & 0x0F) << 12) | ((buf[1] & 0x3F) << 6) | (buf[2] & 0x3F);
    else if (needed == 4) cp = ((buf[0] & 0x07) << 18) | ((buf[1] & 0x3F) << 12) | ((buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);

    *codepoint = cp;
    return 1;
}

/* --- Utility functions --- */
void emit(TokenType t, const char* lexeme) {
    if(lexeme) printf("%s\t%s\n", token_name(t), lexeme);
    else printf("%s\n", token_name(t));
}

void lexer_warning(const char* msg) {
    warning_count++;
    fprintf(stderr, "WARNING: %s\n", msg);
}

void lexer_error(const char* msg) {
    error_count++;
    fprintf(stderr, "ERROR: %s\n", msg);
}

int iequals(const char* a, const char* b){
    while(*a && *b){
        if(tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        a++; b++;
    }
    return *a==0 && *b==0;
}

// Check if codepoint is alphabetic (ASCII or Latin-1 supplement)
int is_alpha_cp(unsigned int cp) {
    return isalpha(cp) || (cp >= 0xC0 && cp <= 0xFF);
}

TokenType keyword_or_id(const char* s) {
    if (iequals(s, "LEIA")) return T_KW_LEIA;
    if (iequals(s, "ESCREVA")) return T_KW_ESCREVA;
    if (iequals(s, "SE")) return T_KW_SE;
    if (iequals(s, "FIM")) return T_KW_FIM;
    if (iequals(s, "ENQUANTO")) return T_KW_ENQUANTO;

    if (iequals(s, "SENAO") || iequals(s, "SENÃO")) return T_KW_SENAO;
    if (iequals(s, "ENTAO") || iequals(s, "ENTÃO")) return T_KW_ENTAO;
    if (iequals(s, "FACA") || iequals(s, "FAÇA")) return T_KW_FACA;

    // Validate identifier
    size_t len = strlen(s);
    if (len >= 1 && len <= 3) {
        for (size_t i = 0; i < len; i++) {
            if (!(s[i] >= 'a' && s[i] <= 'z')) return T_ERROR;
        }
        return T_ID;
    }
    return T_ERROR;
}

/* --- Lexer main loop --- */
int main(int argc, char** argv){
    if(argc < 2){ fprintf(stderr,"Usage: %s file\n", argv[0]); return 1; }
    FILE* f = fopen(argv[1],"r");
    if(!f){ perror("fopen"); return 1; }

    unsigned char utf8buf[5];
    unsigned int cp;
    int len;
    char lexeme[512];

    while(1){
        int status = utf8_next(f, utf8buf, &len, &cp);
        if(status == 0){ emit(T_EOF,NULL); break; }
        if(status == -1){
            lexer_error("Invalid UTF-8 sequence");
            emit(T_ERROR,"utf8");
            continue; // Skip this character and continue
        }

        // whitespace
        if(cp==' ' || cp=='\t' || cp=='\n' || cp=='\r') continue;

        // comment
        if(cp=='['){
            int closed = 0;
            while(utf8_next(f, utf8buf, &len, &cp)){
                if(cp==']'){ closed = 1; break; }
            }
            if(!closed){
                lexer_error("Unterminated comment (missing ']')");
                emit(T_ERROR,"comment");
                // Continue parsing after EOF in comment
            }
            continue;
        }

        // string literal (ASCII ' or curly ' ')
        if(cp=='\'' || cp==0x2018 || cp==0x2019){
            int closed = 0;
            size_t i=0;
            while(utf8_next(f, utf8buf, &len, &cp)){
                if(cp=='\'' || cp==0x2018 || cp==0x2019){ closed=1; break; }
                if(i+len < sizeof(lexeme)-1){
                    memcpy(&lexeme[i], utf8buf, len);
                    i += len;
                }
            }
            lexeme[i]=0;
            if(!closed){
                lexer_error("Unterminated string literal (missing closing quote)");
                emit(T_ERROR,"string");
                // Continue parsing after EOF in string
            }
            emit(T_STRING, lexeme);
            continue;
        }

        // digits
        if(isdigit(cp)){
            lexeme[0] = (char)cp;
            int i=1;
            while(utf8_next(f, utf8buf, &len, &cp)){
                if(!isdigit(cp)){ fseek(f, -len, SEEK_CUR); break; }
                if(i<511) lexeme[i++] = (char)cp;
            }
            lexeme[i]=0;
            emit(T_NUM, lexeme);
            continue;
        }

        // identifier / keyword - FIXED to handle UTF-8 letters
        if(is_alpha_cp(cp)){
            int i=0;
            // Store first character
            if(cp < 0x80){
                lexeme[i++] = (char)cp;
            } else {
                memcpy(&lexeme[i], utf8buf, len);
                i += len;
            }

            // Continue reading alphabetic characters
            while(utf8_next(f, utf8buf, &len, &cp)){
                if(!is_alpha_cp(cp)){
                    fseek(f,-len,SEEK_CUR);
                    break;
                }
                if(i+len < sizeof(lexeme)-1){
                    memcpy(&lexeme[i], utf8buf, len);
                    i += len;
                }
            }
            lexeme[i]=0;

            TokenType t = keyword_or_id(lexeme);
            if(t==T_ERROR){
                lexer_error("Invalid identifier (must be 1-3 lowercase letters)");
                fprintf(stderr, "  Found: '%s'\n", lexeme);
                emit(T_ERROR,lexeme);
                continue; // Continue lexing after invalid identifier
            }
            emit(t, lexeme);
            continue;
        }

        // symbols
        if(cp==':'){
            unsigned char nextbuf[5];
            unsigned int nextcp;
            int nextlen;
            if(utf8_next(f, nextbuf, &nextlen, &nextcp) && nextcp=='='){
                emit(T_ASSIGN, ":=");
            } else {
                if(nextlen) fseek(f, -nextlen, SEEK_CUR);
                lexer_error("':' must be followed by '=' (use ':=' for assignment)");
                emit(T_ERROR,":");
                continue; // Continue lexing after invalid ':'
            }
            continue;
        }

        switch(cp){
            case '+': emit(T_PLUS, "+"); break;
            case '-': emit(T_MINUS, "-"); break;
            case '*': emit(T_TIMES, "*"); break;
            case '/': emit(T_DIV, "/"); break;
            case '<': emit(T_LT, "<"); break;
            case '=': emit(T_EQ, "="); break;
            case ',': emit(T_COMMA, ","); break;
            case '(': emit(T_LPAREN, "("); break;
            case ')': emit(T_RPAREN, ")"); break;
            default:
                char msg[128];
                snprintf(msg, sizeof(msg), "Unexpected character (U+%04X) - skipping", cp);
                lexer_warning(msg);
                fprintf(stderr, "  Character: ");
                for(int i = 0; i < len; i++) {
                    fprintf(stderr, "\\x%02X", utf8buf[i]);
                }
                fprintf(stderr, "\n");
                // Don't emit token, just skip and continue
                continue;
        }
    }

    fclose(f);
    
    // Print summary
    if(error_count > 0 || warning_count > 0) {
        fprintf(stderr, "\n=== Lexical Analysis Summary ===\n");
        fprintf(stderr, "Errors:   %d\n", error_count);
        fprintf(stderr, "Warnings: %d\n", warning_count);
    }
    
    return (error_count > 0) ? 1 : 0;
}

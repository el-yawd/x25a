// lexer.c
// Simple DFA-based lexer for X25a.
// Usage: ./lexer input.x25a > tokens.txt

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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
    case T_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

void emit(TokenType t, const char* lexeme) {
    if(lexeme)
        printf("%s\t%s\n", token_name(t), lexeme);
    else
        printf("%s\n", token_name(t));
}

/* Helper: case-insensitive compare for keywords */
int iequals(const char* a, const char* b){
    while(*a && *b){
        if(tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a==0 && *b==0;
}

/* Check if lexeme is keyword (we accept ASCII versions) */
TokenType keyword_or_id(const char* s){
    if(iequals(s, "LEIA")) return T_KW_LEIA;
    if(iequals(s, "ESCREVA")) return T_KW_ESCREVA;
    if(iequals(s, "SE")) return T_KW_SE;
    if(iequals(s, "ENTAO") || iequals(s, "ENTÃO")) return T_KW_ENTAO;
    if(iequals(s, "SENAO") || iequals(s, "SENÃO")) return T_KW_SENAO;
    if(iequals(s, "FIM")) return T_KW_FIM;
    if(iequals(s, "FACA") || iequals(s, "FAÇA")) return T_KW_FACA;
    if(iequals(s, "ENQUANTO")) return T_KW_ENQUANTO;
    /* else ID: we require lowercase id per spec, but be lenient:
       emit ID if all letters and length <= 3 and all lowercase. */
    int len = strlen(s);
    if(len >=1 && len <= 3){
        for(int i=0;i<len;i++){
            if(!isalpha((unsigned char)s[i])) return T_ERROR;
            if(!islower((unsigned char)s[i])) return T_ERROR; // strictly lowercase
        }
        return T_ID;
    }
    return T_ERROR;
}

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Usage: %s file\n", argv[0]); exit(1);
    }
    FILE* f = fopen(argv[1], "r");
    if(!f){ perror("fopen"); exit(1); }

    int c;
    while((c = fgetc(f)) != EOF){
        // skip whitespace
        if(isspace(c)) continue;

        if(isalpha(c)){
            // read identifier/keyword (letters only)
            char buf[128]; int i=0;
            buf[i++] = c;
            while(i < 127){
                int nc = fgetc(f);
                if(nc == EOF) break;
                if(isalpha(nc)){
                    buf[i++] = nc;
                } else {
                    ungetc(nc, f);
                    break;
                }
            }
            buf[i] = 0;
            TokenType t = keyword_or_id(buf);
            if(t == T_ERROR){
                fprintf(stderr, "Lexical error: invalid identifier/keyword '%s'\n", buf);
                emit(T_ERROR, buf);
                fclose(f);
                return 1;
            } else {
                emit(t, buf);
            }
            continue;
        }

        if(isdigit(c)){
            char buf[128]; int i=0;
            buf[i++] = c;
            while(i < 127){
                int nc = fgetc(f);
                if(nc == EOF) break;
                if(isdigit(nc)) buf[i++] = nc;
                else { ungetc(nc, f); break; }
            }
            buf[i] = 0;
            emit(T_NUM, buf);
            continue;
        }

        switch(c){
            case ':': {
                int nc = fgetc(f);
                if(nc == '=') { emit(T_ASSIGN, ":="); }
                else { fprintf(stderr,"Lexical error: ':' not followed by '='\n"); emit(T_ERROR, ":"); if(nc!=EOF) ungetc(nc,f); fclose(f); return 1; }
                break;
            }
            case '+': emit(T_PLUS, "+"); break;
            case '-': emit(T_MINUS, "-"); break;
            case '*': emit(T_TIMES, "*"); break;
            case '/': emit(T_DIV, "/"); break;
            case '<': emit(T_LT, "<"); break;
            case '=': emit(T_EQ, "="); break;
            case ',': emit(T_COMMA, ","); break;
            case '(': emit(T_LPAREN, "("); break;
            case ')': emit(T_RPAREN, ")"); break;
            case '\'': {
                // string literal until next '
                char buf[1024]; int i=0;
                int closed = 0;
                while(1){
                    int nc = fgetc(f);
                    if(nc == EOF){ fprintf(stderr,"Lexical error: unterminated string\n"); emit(T_ERROR, "unterminated string"); fclose(f); return 1; }
                    if(nc == '\''){ closed = 1; break; }
                    if(i < 1023) buf[i++] = nc;
                }
                buf[i] = 0;
                if(!closed){ fprintf(stderr,"Lexical error: unterminated string\n"); emit(T_ERROR, "unterminated string"); fclose(f); return 1; }
                emit(T_STRING, buf);
                break;
            }
            case '[': {
                // comment until ]
                int closed = 0;
                while(1){
                    int nc = fgetc(f);
                    if(nc == EOF){ fprintf(stderr,"Lexical error: unterminated comment\n"); emit(T_ERROR, "unterminated comment"); fclose(f); return 1; }
                    if(nc == ']'){ closed = 1; break; }
                    // else keep skipping
                }
                // comments are discarded - do not emit token
                break;
            }
            default:
                fprintf(stderr, "Lexical error: unexpected character '%c' (0x%02x)\n", (char)c, c);
                emit(T_ERROR, NULL);
                fclose(f);
                return 1;
        }
    }

    emit(T_EOF, NULL);
    fclose(f);
    return 0;
}

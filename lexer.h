#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define MAX_ID_LEN    64
#define MAX_NUM_LEN   32
#define MAX_STR_LEN   256
#define MAX_CONSTS    1024
#define MAX_NAMES     1024
#define HASH_SIZE     211

typedef enum {
    /* palavras reservadas (em ordem alfabetica) */
    ARRAY, BOOLEAN, BREAK, CHAR, CONTINUE, DO, ELSE, FALSE, FUNCTION, IF,
    INTEGER, OF, STRING, STRUCT, TRUE, TYPE, VAR, WHILE,

    /* simbolos */
    COLON, SEMI_COLON, COMMA, EQUALS, LEFT_SQUARE, RIGHT_SQUARE,
    LEFT_BRACES, RIGHT_BRACES, LEFT_PARENTHESIS, RIGHT_PARENTHESIS, AND,
    OR, LESS_THAN, GREATER_THAN, LESS_OR_EQUAL, GREATER_OR_EQUAL,
    NOT_EQUAL, EQUAL_EQUAL, PLUS, PLUS_PLUS, MINUS, MINUS_MINUS, TIMES,
    DIVIDE, DOT, NOT,

    /* tokens regulares */
    CHARACTER, NUMERAL, STRINGVAL, ID,

    /* fim de arquivo e token desconhecido */
    ENDOFFILE, UNKNOWN
} t_token;

/* constante do pool: 0-char, 1-int, 2-string */
typedef struct {
    unsigned char type;
    union {
        char  cVal;
        int   nVal;
        char *sVal;
    } _;
} t_const;

/* estado publico do analisador (cap. 2.2) */
extern t_token token;
extern int     tokenSecundario;
extern int     nLine;       /* linha corrente do arquivo   */
extern int     nTokenLine;  /* linha onde o token comecou  */

/* interface */
void     initLexer(FILE *f);
t_token  nextToken(void);

t_token  searchKeyWord(char *name);
int      searchName(char *name);
char    *getName(int n);

int   addCharConst(char c);
int   addIntConst(int n);
int   addStringConst(char *s);
char  getCharConst(int n);
int   getIntConst(int n);
char *getStringConst(int n);

const char *tokenName(t_token t);

#endif /* LEXER_H */

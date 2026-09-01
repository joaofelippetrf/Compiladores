/* Analisador Lexico da Simple Script Language
 * Implementa o automato finito da Figura 2.1 (cap. 2.2). */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

t_token token;
int     tokenSecundario;
int     nLine = 1;
int     nTokenLine = 1;

static FILE *fp        = NULL;
static char  nextChar  = '\x20';  /* garante o funcionamento na 1a chamada */
static int   eofFound  = 0;

/* ------------------------------------------------------------------ */
/* tabela de palavras reservadas: ordenada, busca binaria             */
/* ------------------------------------------------------------------ */

static const char *keywords[] = {
    "array", "boolean", "break", "char", "continue", "do", "else",
    "false", "function", "if", "integer", "of", "string", "struct",
    "true", "type", "var", "while"
};
#define NUM_KEYWORDS ((int)(sizeof(keywords) / sizeof(keywords[0])))

t_token searchKeyWord(char *name)
{
    int lo = 0, hi = NUM_KEYWORDS - 1;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(name, keywords[mid]);

        if (cmp == 0) return (t_token)mid;   /* posicao == valor do token */
        if (cmp < 0)  hi = mid - 1;
        else          lo = mid + 1;
    }
    return ID;
}

/* ------------------------------------------------------------------ */
/* tabela de nomes (hash encadeado): token secundario dos ID          */
/* ------------------------------------------------------------------ */

typedef struct s_name {
    char          *name;
    int            id;
    struct s_name *next;
} t_name;

static t_name *hashTable[HASH_SIZE];
static char   *vNames[MAX_NAMES];
static int     nNumNames = 0;

static unsigned hash(const char *s)
{
    unsigned h = 0;
    while (*s) h = h * 31u + (unsigned char)*s++;
    return h % HASH_SIZE;
}

int searchName(char *name)
{
    unsigned h = hash(name);
    t_name  *p;

    for (p = hashTable[h]; p != NULL; p = p->next)
        if (strcmp(p->name, name) == 0)
            return p->id;                 /* ja inserido: mesma posicao */

    if (nNumNames >= MAX_NAMES) return -1;

    p        = (t_name *)malloc(sizeof(t_name));
    p->name  = strdup(name);
    p->id    = nNumNames;
    p->next  = hashTable[h];
    hashTable[h] = p;

    vNames[nNumNames] = p->name;
    return nNumNames++;
}

char *getName(int n)
{
    return (n >= 0 && n < nNumNames) ? vNames[n] : NULL;
}

/* ------------------------------------------------------------------ */
/* pool de constantes                                                  */
/* ------------------------------------------------------------------ */

static t_const vConsts[MAX_CONSTS];
static int     nNumConsts = 0;

int addCharConst(char c)
{
    if (nNumConsts >= MAX_CONSTS) return -1;
    vConsts[nNumConsts].type    = 0;
    vConsts[nNumConsts]._.cVal  = c;
    return nNumConsts++;
}

int addIntConst(int n)
{
    if (nNumConsts >= MAX_CONSTS) return -1;
    vConsts[nNumConsts].type    = 1;
    vConsts[nNumConsts]._.nVal  = n;
    return nNumConsts++;
}

int addStringConst(char *s)
{
    if (nNumConsts >= MAX_CONSTS) return -1;
    vConsts[nNumConsts].type    = 2;
    vConsts[nNumConsts]._.sVal  = strdup(s);
    return nNumConsts++;
}

char getCharConst(int n)
{
    return (n >= 0 && n < nNumConsts && vConsts[n].type == 0)
           ? vConsts[n]._.cVal : '\0';
}

int getIntConst(int n)
{
    return (n >= 0 && n < nNumConsts && vConsts[n].type == 1)
           ? vConsts[n]._.nVal : 0;
}

char *getStringConst(int n)
{
    return (n >= 0 && n < nNumConsts && vConsts[n].type == 2)
           ? vConsts[n]._.sVal : NULL;
}

/* ------------------------------------------------------------------ */
/* leitura do arquivo de entrada                                       */
/* ------------------------------------------------------------------ */

static char readChar(void)
{
    int c = fgetc(fp);

    if (c == EOF) { eofFound = 1; return '\0'; }
    if (c == '\n') nLine++;
    return (char)c;
}

void initLexer(FILE *f)
{
    fp       = f;
    nextChar = '\x20';
    eofFound = 0;
    nLine    = 1;
    nTokenLine = 1;
}

/* ------------------------------------------------------------------ */
/* automato finito                                                     */
/* ------------------------------------------------------------------ */

t_token nextToken(void)
{
    /* loop do estado inicial: pula separadores e comentarios */
    for (;;) {
        while (!eofFound && isspace((unsigned char)nextChar))
            nextChar = readChar();

        if (nextChar != '/') break;

        nextChar = readChar();             /* olha o caractere seguinte */
        if (nextChar == '/') {             /* comentario de linha       */
            while (!eofFound && nextChar != '\n')
                nextChar = readChar();
        } else if (nextChar == '*') {      /* comentario de bloco       */
            char prev = '\0';
            nextChar = readChar();
            while (!eofFound && !(prev == '*' && nextChar == '/')) {
                prev     = nextChar;
                nextChar = readChar();
            }
            if (!eofFound) nextChar = readChar();
        } else {
            token = DIVIDE;                /* era mesmo uma divisao     */
            return token;
        }
    }

    if (eofFound) {
        token = ENDOFFILE;
        return token;
    }

    nTokenLine      = nLine;
    tokenSecundario = -1;

    /* --- identificadores e palavras reservadas --- */
    if (isalpha((unsigned char)nextChar)) {
        char text[MAX_ID_LEN + 1];
        int  i = 0;

        do {
            if (i < MAX_ID_LEN) text[i++] = nextChar;
            nextChar = readChar();
        } while (!eofFound &&
                 (isalnum((unsigned char)nextChar) || nextChar == '_'));
        text[i] = '\0';

        token = searchKeyWord(text);
        if (token == ID)
            tokenSecundario = searchName(text);
        return token;
    }

    /* --- numeros inteiros --- */
    if (isdigit((unsigned char)nextChar)) {
        char numeral[MAX_NUM_LEN + 1];
        int  i = 0;

        do {
            if (i < MAX_NUM_LEN) numeral[i++] = nextChar;
            nextChar = readChar();
        } while (!eofFound && isdigit((unsigned char)nextChar));
        numeral[i] = '\0';

        token           = NUMERAL;
        tokenSecundario = addIntConst(atoi(numeral));
        return token;
    }

    /* --- literais string --- */
    if (nextChar == '"') {
        char str[MAX_STR_LEN + 1];
        int  i = 0;

        nextChar = readChar();                 /* pula a aspa inicial */
        while (!eofFound && nextChar != '"') {
            if (i < MAX_STR_LEN) str[i++] = nextChar;
            nextChar = readChar();
        }
        str[i] = '\0';

        if (eofFound) { token = UNKNOWN; return token; }
        nextChar = readChar();                 /* pula a aspa final   */

        token           = STRINGVAL;
        tokenSecundario = addStringConst(str);
        return token;
    }

    /* --- demais simbolos --- */
    switch (nextChar) {

    case '\'': {
        char c   = readChar();
        nextChar = readChar();                 /* deve ser a aspa final */
        if (eofFound || nextChar != '\'') { token = UNKNOWN; return token; }
        nextChar        = readChar();
        token           = CHARACTER;
        tokenSecundario = addCharConst(c);
        break;
    }

    case ':': nextChar = readChar(); token = COLON;             break;
    case ';': nextChar = readChar(); token = SEMI_COLON;        break;
    case ',': nextChar = readChar(); token = COMMA;             break;
    case '[': nextChar = readChar(); token = LEFT_SQUARE;       break;
    case ']': nextChar = readChar(); token = RIGHT_SQUARE;      break;
    case '{': nextChar = readChar(); token = LEFT_BRACES;       break;
    case '}': nextChar = readChar(); token = RIGHT_BRACES;      break;
    case '(': nextChar = readChar(); token = LEFT_PARENTHESIS;  break;
    case ')': nextChar = readChar(); token = RIGHT_PARENTHESIS; break;
    case '.': nextChar = readChar(); token = DOT;               break;
    case '*': nextChar = readChar(); token = TIMES;             break;
    case '/': nextChar = readChar(); token = DIVIDE;            break;

    case '+':
        nextChar = readChar();
        if (nextChar == '+') { token = PLUS_PLUS;  nextChar = readChar(); }
        else                   token = PLUS;
        break;

    case '-':
        nextChar = readChar();
        if (nextChar == '-') { token = MINUS_MINUS; nextChar = readChar(); }
        else                   token = MINUS;
        break;

    case '=':
        nextChar = readChar();
        if (nextChar == '=') { token = EQUAL_EQUAL; nextChar = readChar(); }
        else                   token = EQUALS;
        break;

    case '!':
        nextChar = readChar();
        if (nextChar == '=') { token = NOT_EQUAL; nextChar = readChar(); }
        else                   token = NOT;
        break;

    case '<':
        nextChar = readChar();
        if (nextChar == '=') { token = LESS_OR_EQUAL; nextChar = readChar(); }
        else                   token = LESS_THAN;
        break;

    case '>':
        nextChar = readChar();
        if (nextChar == '=') { token = GREATER_OR_EQUAL; nextChar = readChar(); }
        else                   token = GREATER_THAN;
        break;

    case '&':
        nextChar = readChar();
        if (nextChar == '&') { token = AND; nextChar = readChar(); }
        else                   token = UNKNOWN;
        break;

    case '|':
        nextChar = readChar();
        if (nextChar == '|') { token = OR; nextChar = readChar(); }
        else                   token = UNKNOWN;
        break;

    default:
        nextChar = readChar();
        token    = UNKNOWN;
        break;
    }

    return token;
}

/* ------------------------------------------------------------------ */
/* nomes dos tokens, para depuracao                                    */
/* ------------------------------------------------------------------ */

static const char *names[] = {
    "ARRAY", "BOOLEAN", "BREAK", "CHAR", "CONTINUE", "DO", "ELSE", "FALSE",
    "FUNCTION", "IF", "INTEGER", "OF", "STRING", "STRUCT", "TRUE", "TYPE",
    "VAR", "WHILE",
    "COLON", "SEMI_COLON", "COMMA", "EQUALS", "LEFT_SQUARE", "RIGHT_SQUARE",
    "LEFT_BRACES", "RIGHT_BRACES", "LEFT_PARENTHESIS", "RIGHT_PARENTHESIS",
    "AND", "OR", "LESS_THAN", "GREATER_THAN", "LESS_OR_EQUAL",
    "GREATER_OR_EQUAL", "NOT_EQUAL", "EQUAL_EQUAL", "PLUS", "PLUS_PLUS",
    "MINUS", "MINUS_MINUS", "TIMES", "DIVIDE", "DOT", "NOT",
    "CHARACTER", "NUMERAL", "STRINGVAL", "ID",
    "ENDOFFILE", "UNKNOWN"
};

const char *tokenName(t_token t)
{
    return names[t];
}

/* Driver do Analisador Lexico: imprime a sequencia de tokens do arquivo. */

#include <stdio.h>
#include "lexer.h"

int main(int argc, char *argv[])
{
    FILE *f;
    int   nErros = 0;

    if (argc != 2) {
        fprintf(stderr, "uso: %s <arquivo.ssl>\n", argv[0]);
        return 1;
    }

    f = fopen(argv[1], "r");
    if (f == NULL) {
        fprintf(stderr, "erro: nao foi possivel abrir '%s'\n", argv[1]);
        return 1;
    }

    initLexer(f);

    printf("%-5s %-20s %-8s %s\n", "LINHA", "TOKEN", "SEC", "VALOR");
    printf("---------------------------------------------------------\n");

    while (nextToken() != ENDOFFILE) {
        printf("%-5d %-20s ", nTokenLine, tokenName(token));

        switch (token) {
        case ID:
            printf("%-8d %s\n", tokenSecundario, getName(tokenSecundario));
            break;
        case NUMERAL:
            printf("%-8d %d\n", tokenSecundario, getIntConst(tokenSecundario));
            break;
        case CHARACTER:
            printf("%-8d '%c'\n", tokenSecundario, getCharConst(tokenSecundario));
            break;
        case STRINGVAL:
            printf("%-8d \"%s\"\n", tokenSecundario,
                   getStringConst(tokenSecundario));
            break;
        case UNKNOWN:
            printf("%-8s %s\n", "-", "<lexema invalido>");
            nErros++;
            break;
        default:
            printf("%-8s\n", "-");
            break;
        }
    }

    fclose(f);

    if (nErros > 0) {
        printf("\n%d lexema(s) invalido(s) encontrado(s).\n", nErros);
        return 1;
    }
    printf("\nAnalise lexica concluida sem erros.\n");
    return 0;
}

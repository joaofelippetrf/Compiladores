# Analisador Léxico da Simple Script Language

Implementação em C da primeira fase de um compilador para a **Simple Script Language (SSL)**,
uma linguagem imperativa de sintaxe inspirada em ECMAScript, com tipagem estática, tipos
agregados (vetores e estruturas) e funções.

O analisador lê um arquivo-fonte caractere a caractere e devolve, a cada chamada, o próximo
*token* reconhecido, acompanhado de um *token secundário* que identifica qual lexema em
particular foi lido.

## Compilação e uso

```
make                    # compila com -Wall -Wextra -std=c99 -pedantic
./lexico exemplo.ssl    # analisa um arquivo
make test               # compila e roda sobre exemplo.ssl
make clean              # remove objetos e binário
```

Saída para um trecho de `exemplo.ssl`:

```
LINHA TOKEN                SEC      VALOR
---------------------------------------------------------
3     TYPE                 -
3     ID                   0        Ponto
3     EQUALS               -
3     STRUCT               -
3     LEFT_BRACES          -
3     ID                   1        x
3     COMMA                -
3     ID                   2        y
3     COLON                -
3     INTEGER              -
3     RIGHT_BRACES         -
3     SEMI_COLON           -
```

Cada linha traz a linha do arquivo-fonte, o nome do token, o token secundário (quando
existe) e o valor do lexema.

## Arquivos

| Arquivo | Papel |
|---|---|
| `gramatica.txt` | Gramática Livre de Contexto da SSL, definições regulares e alfabeto de tokens |
| `lexer.h` | Enumeração `t_token`, estrutura `t_const`, limites e interface pública |
| `lexer.c` | Autômato finito, tabela de palavras reservadas, tabela de nomes e pool de constantes |
| `main.c` | Driver: percorre o arquivo e imprime a tabela de tokens |
| `Makefile` | Compilação, execução do exemplo e limpeza |
| `exemplo.ssl` | Programa de teste que exercita as construções da linguagem |
| `documentacao.pdf` | Documentação completa do projeto |

## A linguagem

A gramática completa está em `gramatica.txt`. Em resumo, um programa é uma lista de
declarações externas — funções, tipos ou variáveis:

```
P    -> LDE
LDE  -> LDE DE  |  DE
DE   -> DF  |  DT  |  DV
```

Os tipos primitivos são `integer`, `char`, `boolean` e `string`, e novos tipos podem ser
declarados como vetores, estruturas ou sinônimos:

```
DT   -> 'type' ID '=' 'array' '[' NUM ']' 'of' T
      | 'type' ID '=' 'struct' '{' DC '}'
      | 'type' ID '=' T
```

Os comandos disponíveis são `if`/`else`, `while`, `do…while`, blocos, atribuição, `break` e
`continue`. Nas expressões, a precedência dos operadores está codificada na própria estrutura
das regras — cada nível só pode conter os níveis mais internos:

```
E    -> E '&&' L  |  E '||' L  |  L            (lógicos)
L    -> L '<' R | L '>' R | ... | L '!=' R | R (relacionais)
R    -> R '+' Y  |  R '-' Y  |  Y              (aditivos)
Y    -> Y '*' F  |  Y '/' F  |  F              (multiplicativos)
F    -> LV | '++' LV | '(' E ')' | ID '(' LE ')' | NUM | STR | ...
```

A recursão à esquerda garante associatividade à esquerda. Como está, a gramática é adequada
a um analisador sintático ascendente.

## Os tokens

Tudo que aparece entre aspas simples na gramática é um lexema elementar — palavra reservada
ou símbolo. Os demais elementos são descritos por expressões regulares:

```
letra  = 'a'..'z' + 'A'..'Z'
digito = '0'..'9'

Id     = letra . ( letra + digito + '_' )*
n      = digito . digito*
c      = "'" . any . "'"
s      = '"' . any* . '"'
```

A enumeração `t_token`, em `lexer.h`, reúne 50 valores:

- **18 palavras reservadas**, em ordem alfabética: `ARRAY`, `BOOLEAN`, `BREAK`, `CHAR`,
  `CONTINUE`, `DO`, `ELSE`, `FALSE`, `FUNCTION`, `IF`, `INTEGER`, `OF`, `STRING`, `STRUCT`,
  `TRUE`, `TYPE`, `VAR`, `WHILE`
- **26 símbolos**, incluindo os compostos `++ -- == != <= >= && ||`
- **4 tokens regulares**: `CHARACTER`, `NUMERAL`, `STRINGVAL`, `ID`
- **2 de controle**: `ENDOFFILE` e `UNKNOWN`

A ordem alfabética das palavras reservadas não é decorativa: como elas ocupam as primeiras
posições da enumeração, **a posição de uma palavra no vetor ordenado é exatamente o valor do
seu token**. Isso permite reconhecê-las com uma busca binária que devolve o índice
encontrado, sem nenhuma tabela de tradução adicional.

## Interface

```c
void     initLexer(FILE *f);   /* prepara a leitura de um arquivo   */
t_token  nextToken(void);      /* devolve o proximo token           */

extern t_token token;           /* token corrente                    */
extern int     tokenSecundario; /* qual lexema, quando aplicavel     */
extern int     nTokenLine;      /* linha em que o token comeca       */
```

`nextToken()` devolve o token e, como efeito, atualiza `token`, `tokenSecundario` e
`nTokenLine`. O consumidor natural dessa interface é um analisador sintático, que a chama sob
demanda enquanto constrói a árvore de derivação.

## Estruturas de dados

### Tabela de palavras reservadas — `searchKeyWord()`

Vetor estático de 18 *strings* ordenadas, percorrido por busca binária. Se a palavra lida
estiver no vetor, sua posição é o token; caso contrário, o lexema é um identificador criado
pelo programador e a função devolve `ID`.

### Tabela de nomes — `searchName()`

Todos os identificadores compartilham o mesmo token `ID`, o que criaria ambiguidade para as
fases seguintes do compilador. O token secundário resolve isso: cada identificador distinto
recebe um inteiro correspondente à ordem em que apareceu pela primeira vez no arquivo. O
primeiro identificador lido recebe 0, o segundo 1, e assim por diante; uma nova ocorrência do
mesmo nome devolve o número já atribuído.

A tabela é uma *hash* com encadeamento externo (211 posições, função polinomial de base 31),
com busca e inserção em tempo praticamente constante. Um vetor paralelo permite recuperar o
lexema a partir do número, via `getName()`.

### Pool de constantes

Os literais de caractere, inteiro e *string* são armazenados durante a compilação em uma
única tabela, de modo que a numeração das constantes seja global. A união discriminada
acomoda os três tipos na mesma estrutura:

```c
typedef struct {
    unsigned char type;          /* 0-char, 1-int, 2-string */
    union {
        char  cVal;
        int   nVal;
        char *sVal;
    } _;
} t_const;
```

`addCharConst()`, `addIntConst()` e `addStringConst()` inserem e devolvem a posição ocupada —
que passa a ser o token secundário do literal. `getCharConst()`, `getIntConst()` e
`getStringConst()` fazem o caminho inverso, verificando o campo `type` antes de devolver o
valor.

## O autômato

### O caractere de lookahead

Este é o ponto central da implementação. Um caractere que não pertence ao token corrente já
foi lido do arquivo e não pode ser descartado, porque pertence ao **próximo** token. Na
expressão `a+b`, é a leitura do `'+'` que informa ao analisador que o identificador `a`
terminou — e esse `'+'` precisa sobreviver até a chamada seguinte.

Por isso o analisador mantém permanentemente um caractere adiantado, em `nextChar`,
inicializado com um espaço para que a primeira chamada de `nextToken()` funcione: o espaço é
consumido pelo laço de separadores, que então lê o primeiro caractere real do arquivo.

```c
static char nextChar = '\x20';  /* garante o funcionamento na 1a chamada */
```

### Estado inicial

O estado inicial salta espaços, tabulações e quebras de linha. O mesmo laço trata
comentários `//` e `/* */`. O caractere `'/'` é ambíguo — pode iniciar um comentário ou ser o
operador de divisão — e a desambiguação exige olhar o caractere seguinte.

### Os quatro ramos

Passados os separadores, o caractere corrente decide qual estado final será perseguido:

- **Identificadores e palavras reservadas** — se o caractere é uma letra, acumula letras,
  dígitos e `_` até que o caractere lido não pertença mais ao lexema. O texto acumulado passa
  por `searchKeyWord()`; se o resultado for `ID`, `searchName()` fornece o token secundário.
- **Numerais** — se é um dígito, acumula dígitos e insere o valor no pool de constantes.
- **Strings** — se é `"`, consome tudo até a aspa de fechamento. Um fim de arquivo antes do
  fechamento produz `UNKNOWN`.
- **Símbolos** — um `switch` sobre o caractere. Os de um caractere são imediatos; os de dois
  exigem consultar o lookahead:

```c
case '+':
    nextChar = readChar();
    if (nextChar == '+') { token = PLUS_PLUS;  nextChar = readChar(); }
    else                   token = PLUS;
    break;
```

Um `&` ou `|` isolado não forma lexema válido na linguagem e produz `UNKNOWN`.

## Tratamento de erros

Lexemas inválidos não interrompem a análise: o analisador emite `UNKNOWN`, consome o
caractere ofensor e prossegue, de modo que uma única execução reporte todos os erros do
arquivo. O driver conta as ocorrências e encerra com código de saída 1 se houver alguma.

```
2     ID                   0        a
2     UNKNOWN              -        <lexema invalido>
2     NUMERAL              0        3
2     SEMI_COLON           -

1 lexema(s) invalido(s) encontrado(s).
```

A linha reportada é a do **início** do lexema. Isso importa porque o analisador está sempre um
caractere à frente: um token no fim da linha já teria feito o contador avançar, e o erro
apareceria na linha seguinte.

## Limites

Definidos em `lexer.h` e ajustáveis por recompilação:

| Constante | Valor | Significado |
|---|---|---|
| `MAX_ID_LEN` | 64 | comprimento máximo de um identificador |
| `MAX_NUM_LEN` | 32 | dígitos em um numeral |
| `MAX_STR_LEN` | 256 | caracteres em um literal string |
| `MAX_CONSTS` | 1024 | constantes no pool |
| `MAX_NAMES` | 1024 | identificadores distintos |
| `HASH_SIZE` | 211 | posições da tabela de nomes |

Lexemas mais longos que o limite são truncados, sem interromper a análise.

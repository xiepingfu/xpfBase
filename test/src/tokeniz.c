#include <ctype.h>
#include <string.h>
#include "tokeniz.h"
#include "parser_node.h"
#include "utils.h"

typedef struct Keyword
{
    char *name;
    int n;
    TokenType tokenType;
} Keyword;

static Keyword keywordTable[] = {
    {"AND", 0, TK_AND},
    {"ASC", 0, TK_ASC},
    {"BY", 0, TK_BY},
    {"CREATE", 0, TK_CREATE},
    {"DELETE", 0, TK_DELETE},
    {"DESC", 0, TK_DESC},
    {"DROP", 0, TK_DROP},
    {"FROM", 0, TK_FROM},
    {"GROUP", 0, TK_GROUP},
    {"INDEX", 0, TK_INDEX},
    {"INSERT", 0, TK_INSERT},
    {"INTO", 0, TK_INTO},
    {"ON", 0, TK_ON},
    {"OR", 0, TK_OR},
    {"ORDER", 0, TK_ORDER},
    {"SELECT", 0, TK_SELECT},
    {"SET", 0, TK_SET},
    {"TABLE", 0, TK_TABLE},
    {"VALUES", 0, TK_VALUES},
    {"WHERE", 0, TK_WHERE},
    {"INT", 0, TK_INT},
    {"FLOAT", 0, TK_FLOAT},
    {"STRING", 0, TK_STRING},
    {"DATABASE", 0, TK_DATABASE},
    {"USE", 0, TK_USE},
};

TokenType GetKeywordCode(const char *word, int n)
{
    int m = sizeof(keywordTable) / sizeof(keywordTable[0]);
    if (keywordTable[0].n == 0)
    {
        for (int i = 0; i < m; ++i)
        {
            keywordTable[i].n = strlen(keywordTable[i].name);
        }
    }
    for (int i = 0; i < m; ++i)
    {
        if (StringCompareIC(word, n, keywordTable[i].name, keywordTable[i].n) == 0)
        {
            return keywordTable[i].tokenType;
        }
    }
    return TK_ID;
}

Token GetToken(XBParser *xbParser)
{
    Token token;
    int i = 0;
    while (i >= 0 && xbParser->lastSql[i] != 0 && isspace(xbParser->lastSql[i]))
    {
        ++i;
    }
    token.sql = &xbParser->lastSql[i];
    switch (xbParser->lastSql[i])
    {
    case '-':
    {
        if (xbParser->lastSql[i + 1] == 0)
        {
            token.n = 0;
            token.tokenType = TK_ILLEGAL;
            break;
        }
        token.n = 1;
        token.tokenType = TK_MINUS;
        break;
    }
    case '(':
    {
        token.n = 1;
        token.tokenType = TK_LPAREN;
        break;
    }
    case ')':
    {
        token.n = 1;
        token.tokenType = TK_RPAREN;
        break;
    }
    case ';':
    {
        token.n = 1;
        token.tokenType = TK_SEMICOLON;
        break;
    }
    case '+':
    {
        token.n = 1;
        token.tokenType = TK_PLUS;
        break;
    }
    case '*':
    {
        token.n = 1;
        token.tokenType = TK_STAR;
        break;
    }
    case '/':
    {
        token.n = 1;
        token.tokenType = TK_SLASH;
        break;
    }
    case '=':
    {
        token.n = 1;
        if (xbParser->lastSql[i + 1] == '=')
            token.n = 2;
        token.tokenType = TK_EQ;
        break;
    }
    case '<':
    {
        if (xbParser->lastSql[i + 1] == '=')
        {
            token.n = 2;
            token.tokenType = TK_LE;
        }
        else if (xbParser->lastSql[i + 1] == '>')
        {
            token.n = 2;
            token.tokenType = TK_NE;
        }
        else
        {
            token.n = 1;
            token.tokenType = TK_LT;
        }
        break;
    }
    case '>':
    {
        if (xbParser->lastSql[i + 1] == '=')
        {
            token.n = 2;
            token.tokenType = TK_GE;
        }
        else
        {
            token.n = 1;
            token.tokenType = TK_GT;
        }
        break;
    }
    case '!':
    {
        if (xbParser->lastSql[i + 1] != '=')
        {
            token.n = 0;
            token.tokenType = TK_ILLEGAL;
            break;
        }
        else
        {
            token.n = 2;
            token.tokenType = TK_NE;
        }
        break;
    }
    case '|':
    {
        if (xbParser->lastSql[i + 1] != '|')
        {
            token.n = 0;
            token.tokenType = TK_ILLEGAL;
            break;
        }
        else
        {
            token.n = 2;
            token.tokenType = TK_OR;
            break;
        }
    }
    case ',':
    {
        token.n = 1;
        token.tokenType = TK_COMMA;
        break;
    }
    case '\'':
    case '"':
    {
        char colon = xbParser->lastSql[i];
        int j = i + 1;
        token.sql = &xbParser->lastSql[j];
        for (; xbParser->lastSql[j] != 0; ++j)
        {
            if (xbParser->lastSql[j] == colon)
            {
                break;
            }
        }
        token.n = j - i - 1;
        token.tokenType = VAL_STRING;
        break;
    }
    case '.':
    {
        if (!isdigit(xbParser->lastSql[i + 1]))
        {
            token.n = 1;
            token.tokenType = TK_DOT;
            break;
        }
        // Into the next case
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    {
        token.tokenType = VAL_INT;
        int j = i + 1;
        for (; xbParser->lastSql[j] != 0 && isdigit(xbParser->lastSql[j]); ++j)
        {
        }
        if (xbParser->lastSql[j] == '.')
        {
            ++j;
            while (xbParser->lastSql[j] != 0 && isdigit(xbParser->lastSql[j]))
            {
                ++j;
            }
            token.tokenType = VAL_FLOAT;
        }
        if ((xbParser->lastSql[j] == 'e' || xbParser->lastSql[j] == 'E') &&
            (isdigit(xbParser->lastSql[j + 1]) || ((xbParser->lastSql[j + 1] == '+' || xbParser->lastSql[j + 1] == '-') && isdigit(xbParser->lastSql[j + 2]))))
        {
            j += 2;
            while (xbParser->lastSql[j] != 0 && isdigit(xbParser->lastSql[j]))
            {
                ++j;
            }
        }
        else if (xbParser->lastSql[0] == '.')
        {
            token.tokenType = VAL_FLOAT;
        }
        token.n = j - i;
        break;
    }
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case '_':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    {
        int j = i + 1;
        for (; xbParser->lastSql[j] != 0 && (isalnum(xbParser->lastSql[j]) || xbParser->lastSql[j] == '_'); ++j)
        {
        }
        token.n = j - i;
        token.tokenType = GetKeywordCode(&xbParser->lastSql[i], j - i);
        break;
    }
    default:
    {
        token.n = 0;
        token.tokenType = TK_ILLEGAL;
        break;
    }
    }
    xbParser->lastSql = &xbParser->lastSql[i + token.n];
    return token;
}
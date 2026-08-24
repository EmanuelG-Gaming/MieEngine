#include "vm_assembly.h"
#include "anm_manager.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// TODO: Use label table to translate label strings into addresses.


static const char* TypeToStr(ASM_TOKEN_TYPE type)
{
    switch (type)
    {
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_VAL: return "TOKEN_VAL";
        case TOKEN_NUMBER_LIT: return "TOKEN_NUMBER_LIT";
        case TOKEN_CHAR_LIT: return "TOKEN_CHAR_LIT";
        case TOKEN_STR_LIT: return "TOKEN_STR_LIT";

        case TOKEN_IDENT: return "TOKEN_IDENT";
        case LABEL_T: return "TOKEN_LABEL";
        case REGION_T: return "TOKEN_REGION";

        case KW_END: return "TOKEN_KW_END";
        case KW_PUSH: return "TOKEN_KW_PUSH";
        case KW_POP: return "TOKEN_KW_POP";
        case KW_XCHG: return "TOKEN_KW_XCHG";
        case KW_ADD: return "TOKEN_KW_ADD";
        case KW_SUB: return "TOKEN_KW_SUB";
        case KW_MUL: return "TOKEN_KW_MUL";
        case KW_DIV: return "TOKEN_KW_DIV";

        case KW_AND: return "TOKEN_KW_AND";
        case KW_OR: return "TOKEN_KW_OR";
        case KW_LES: return "TOKEN_KW_LES";
        case KW_LEQ: return "TOKEN_KW_LEQ";
        case KW_GTR: return "TOKEN_KW_GTR";
        case KW_GEQ: return "TOKEN_KW_GEQ";
        case KW_JMP: return "TOKEN_KW_JMP";
        case KW_JZ: return "TOKEN_KW_JZ";

        case KW_JNZ: return "TOKEN_KW_JNZ";
        case KW_CALL: return "TOKEN_KW_CALL";
        case KW_RET: return "TOKEN_KW_RET";
        case KW_GOTO: return "TOKEN_KW_GOTO";
        case KW_PRINT: return "TOKEN_KW_PRINT";

        default: return "TOKEN_UNKNOWN";
    }
}

static int GetNumberOfArguments(ASM_TOKEN_TYPE instr)
{
    switch (instr)
    {
        case KW_END: return 1;
        case KW_PUSH: return 1;
        case KW_POP: return 1;
        case KW_XCHG: return 2;
        case KW_ADD: return 2;
        case KW_SUB: return 2;
        case KW_MUL: return 2;
        case KW_DIV: return 2;
        case KW_AND: return 2;
        case KW_OR: return 2;
        case KW_LES: return 2;
        case KW_LEQ: return 2;
        case KW_GTR: return 2;
        case KW_GEQ: return 2;
        case KW_JMP: return 1;
        case KW_JZ: return 1;
        case KW_JNZ: return 1;
        case KW_CALL: return 1;
        case KW_RET: return 1;
        case KW_PRINT: return 1;
        default: return 0;
    }
}

static inline int IsOpcode(ASM_TOKEN_TYPE t)
{
    return (t >= KW_END && t <= KW_PRINT);
}

// Arguments that can be put in an opcode.
static inline int IsArg(ASM_TOKEN_TYPE t)
{
    return (t >= TOKEN_VAL && t <= TOKEN_IDENT);
}

static inline long int StrToInt(char* str, size_t len)
{
    char buf[32]; // Enough digit count for int.
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;

    _MEMCPY(buf, str, len);
    buf[len] = '\0';

    return strtol(buf, NULL, 10);
}





/*
static void ParseToken(
    ASM_TOKEN* token, b32* isLiteral,
    uintptr_t* args, int* nArgs, int tokenPos)
{
    if (token != NULL)
    {
        if (token->text[0] == '$') {
            // Is value. (i.e. $5 for relative stack index '5')
            *isLiteral = FALSE;
            args[tokenPos] = atoi((const char *) token->text + 1);
        } else {
            // Is literal.
            *isLiteral = TRUE;
            // Check alphanumericity.
            args[tokenPos] = atoi((const char *) token->text);
        }
    }
}
*/


/*
static void TokensToOpcodes(ASM_RESULT* ctx,
    ASM_TOKEN* tokenSlice, int nTokens,
    uintptr_t* args, int* nArgs)
{
    if (tokenSlice == NULL || nTokens <= 0)
    {
        fprintf(stderr, "[ERROR] %s: Tokens are empty!\n", __func__);
        *nArgs = 0;
        return;
    }

    ASM_TOKEN *a = NULL, *b = NULL;
    b32 aLiteral = FALSE, bLiteral = FALSE;
    if (nTokens > 1)
    {
        a = &tokenSlice[1];
    }
    if (nTokens > 2)
    {
        b = &tokenSlice[2];
    }

    //ParseToken(a, &aLiteral, args, nArgs);

    if (a != NULL)
    {
        if (a->text[0] == '$') {
            // Is value. (i.e. $5 for relative stack index '5')
            aLiteral = FALSE;
            args[1] = atoi((const char *) a->text + 1);
        } else {
            // Is literal.
            aLiteral = TRUE;
            args[1] = atoi((const char *) a->text);
        }
    }

    if (b != NULL)
    {
        if (b->text[0] == '$') {
            // Is value.
            bLiteral = FALSE;
            args[2] = atoi((const char *) b->text + 1);
        } else {
            // Is literal.
            bLiteral = TRUE;
            args[2] = atoi((const char *) b->text);
        }
    }


    int nA = 1;
    switch (tokenSlice[0].type)
    {
        case KW_END:
        {
            if (a != NULL)
            {
                if (a->text[0] == '$') {
                    // EX: end $0
                    // Is endV.
                    args[0] = ENDV;
                    args[1] = atoi((const char *) a->text + 1);
                } else {
                    // EX: end 67
                    // Is endL.
                    args[0] = ENDL;
                    args[1] = atoi((const char *) a->text);
                }
            }
            nA += nTokens - 1;
        } break;


        case KW_PUSH:
        {
            args[0] = PUSHL;

            if (a != NULL)
            {
                if (a->text[0] == '$') {
                    // Is pushV.
                    args[0] = PUSHV;
                    args[1] = atoi((const char *) a->text + 1);
                } else {
                    // Is pushL.
                    args[1] = atoi((const char *) a->text);
                }
            }
            nA += nTokens - 1;
        } break;

        default: break;
    }

    *nArgs = nA;
}
*/

// Returns the number of new arguments (micro-operations).
// For example, ret becomes RETL 0
static int TokensToOpcodes(ASM_RESULT* ctx,
    ASM_TOKEN* tokenSlice, int nTokens,
    uintptr_t* opcodes)
{
    if (tokenSlice == NULL || nTokens <= 0)
    {
        fprintf(stderr, "[ERROR] %s: Tokens are empty!\n", __func__);
        return -1;
    }

    ASM_TOKEN *a = NULL, *b = NULL;
    b32 aLiteral = FALSE, bLiteral = FALSE;

    // We use 2 arguments for turning tokens into micro-operations.
    if (nTokens > 1)
    {
        a = &tokenSlice[1];
    }
    if (nTokens > 2)
    {
        b = &tokenSlice[2];
    }

    // Put first token as arg.
    //args[0] = tokenSlice[0];

    uintptr_t tmpArgs[8];
    int argCount = nTokens - 1; // For the args that go to the first token.

    if (a != NULL)
    {
        if (a->type == TOKEN_VAL) {
            // Is value. (i.e. $5 for relative stack index '5')
            aLiteral = FALSE;
            tmpArgs[1] = StrToInt(a->text, a->textLen);
        } else {
            // Is literal.
            aLiteral = TRUE;

            if (a->type == TOKEN_NUMBER_LIT) {
                tmpArgs[1] = StrToInt(a->text, a->textLen);
            } else if (a->type == TOKEN_CHAR_LIT) {
                tmpArgs[1] = a->text[0];
            } else if (a->type == TOKEN_STR_LIT) {
                ANM_Segment* textSegment = ctx->srcVM->textSegment;
                strncpy((char *) textSegment->begin + textSegment->offset,
                        a->text, a->textLen);

                // Put address in the micro-operations.
                tmpArgs[1] = textSegment->begin + textSegment->offset;
                textSegment->offset += a->textLen;
            }
        }
    }

    if (b != NULL)
    {
        if (b->type == TOKEN_VAL) {
            // Is value. (i.e. $5 for relative stack index '5')
            bLiteral = FALSE;
            tmpArgs[2] = StrToInt(b->text, a->textLen);
        } else {
            // Is literal.
            bLiteral = TRUE;

            if (b->type == TOKEN_NUMBER_LIT) {
                tmpArgs[2] = StrToInt(b->text, a->textLen);
            } else if (a->type == TOKEN_CHAR_LIT) {
                tmpArgs[2] = b->text[0];
            } else if (b->type == TOKEN_STR_LIT) {
                ANM_Segment* textSegment = ctx->srcVM->textSegment;

                //_MEMCPY((char *) textSegment->begin + textSegment->offset, b->text, 2);//b->textLen);

                // Put address in the micro-operations.
                tmpArgs[2] = textSegment->begin + textSegment->offset;
                textSegment->offset += b->textLen;
            }
        }
    }



    /*
    if (b != NULL)
    {
        if (b->text[0] == '$') {
            // Is value.
            bLiteral = FALSE;
            args[2] = atoi((const char *) b->text + 1);
        } else {
            // Is literal.
            bLiteral = TRUE;
            args[2] = atoi((const char *) b->text);
        }
    }
    */

    /*
    int nA = 1;
    switch (tokenSlice[0].type)
    {
        case KW_END:
        {
            if (a != NULL)
            {
                if (a->text[0] == '$') {
                    // EX: end $0
                    // Is endV.
                    args[0] = ENDV;
                    args[1] = atoi((const char *) a->text + 1);
                } else {
                    // EX: end 67
                    // Is endL.
                    args[0] = ENDL;
                    args[1] = atoi((const char *) a->text);
                }
            }
            nA += nTokens - 1;
        } break;


        case KW_PUSH:
        {
            args[0] = PUSHL;

            if (a != NULL)
            {
                if (a->text[0] == '$') {
                    // Is pushV.
                    args[0] = PUSHV;
                    args[1] = atoi((const char *) a->text + 1);
                } else {
                    // Is pushL.
                    args[1] = atoi((const char *) a->text);
                }
            }
            nA += nTokens - 1;
        } break;

        default: break;
    }
    */

    int nOpcodes = 0;
    switch (tokenSlice[0].type)
    {
        case KW_END:
        {
            if (argCount > 1)
            {
                // Do semval.
                fprintf(stderr, "[ERROR] %s: Cannot call 'end' with more than 1 argument!\n", __func__);
                fprintf(stderr, "arg count: %d\n", argCount);
                return -1;
            }

            // EX: 'end $9' (end with stack element at index 9).
            // is endV 9.
            // while 'end' is endL 0.
            if (a != NULL)
            {
                if (aLiteral) {
                    opcodes[0] = ENDL;
                    opcodes[1] = tmpArgs[1];
                } else {
                    opcodes[0] = ENDV;
                    opcodes[1] = tmpArgs[1];
                }
            }
            else
            {
                opcodes[0] = ENDL;
                opcodes[1] = 0;
            }

            nOpcodes = 2;
        } break;

        case KW_PUSH:
        {
            if (argCount > 1)
            {
                // Do semval.
                fprintf(stderr, "[ERROR] %s: Cannot call 'push' with more than 1 argument!\n", __func__);
                return -1;
            }

            // e.g. push 5 becomes PUSHL 5
            if (a != NULL)
            {
                if (aLiteral) {
                    opcodes[0] = PUSHL;
                    opcodes[1] = tmpArgs[1];
                } else {
                    // TODO: Do multiple checking.
                    opcodes[0] = PUSHV;
                    opcodes[1] = tmpArgs[1];
                }
            }
            else
            {
                fprintf(stderr, "[ERROR] %s: Cannot push empty variable!\n", __func__);
                return -1;
            }

            nOpcodes = 2;
        } break;

        case KW_POP:
        {
            if (argCount > 1)
            {
                fprintf(stderr, "[ERROR] %s: Cannot call 'pop' with more than 1 argument!\n", __func__);
                return -1;
            }

            if (a != NULL)
            {
                if (aLiteral) {
                    opcodes[0] = POP;
                    opcodes[1] = tmpArgs[1];
                } else {
                    fprintf(stderr, "[ERROR] %s: Cannot pop a variable amount of stack elements!\n", __func__);
                    return -1;
                }
            }
            else
            {
                opcodes[0] = POP;
                opcodes[1] = 1;
            }

            nOpcodes = 2;
        } break;

        case KW_XCHG:
        {
            if (argCount > 2)
            {
                fprintf(stderr, "[ERROR] %s: Cannot call 'xchg' with more than 1 argument!\n", __func__);
                return -1;
            }
            if (aLiteral && bLiteral)
            {
                fprintf(stderr, "[ERROR] %s: Cannot 'xchg' two literals!\n", __func__);
                return -1;
            }
            if (aLiteral != bLiteral)
            {
                fprintf(stderr, "[ERROR] %s: Cannot 'xchg' a variable and a literal!\n", __func__);
                return -1;
            }

            if (a == NULL)
            {
                fprintf(stderr, "[ERROR] %s Called 'xchg' with no arguments!\n", __func__);
                return -1;
            }
            if (b == NULL)
            {
                fprintf(stderr, "[ERROR] %s Called 'xchg' with no arguments!\n", __func__);
                return -1;
            }


            // 'xchg' works by pushing the first argument, and then using the XCHG opcode
            // to exchange the stack top with the second argument as the stack offset
            // So if you have xchg $5 $2, it will do PUSHV 5 XCHG 2

            // It might create some stack rot because of the PUSHV instruction copying stack elements.

            int idx = 0;
            if (b != NULL)
            {
                opcodes[idx++] = PUSHV;
                opcodes[idx++] = tmpArgs[1]; // or 2.
            }

            opcodes[idx++] = XCHG;
            opcodes[idx++] = tmpArgs[2]; // or 1.

            nOpcodes = idx;
        } break;

        case KW_ADD:
        {
            if (argCount > 2)
            {
                fprintf(stderr, "[ERROR] %s: Cannot call 'add' with more than 2 arguments!\n", __func__);
                return -1;
            }
            if (a == NULL || b == NULL)
            {
                fprintf(stderr, "%s: Cannot call 'add' with less than 2 arguments!\n", __func__);
                return -1;
            }

            int idx = 0;
            opcodes[idx++] = (aLiteral) ? PUSHL : PUSHV;
            opcodes[idx++] = tmpArgs[1];
            opcodes[idx++] = (bLiteral) ? PUSHL : PUSHV;
            opcodes[idx++] = tmpArgs[2];

            // Add 2 stack top elements.
            opcodes[idx++] = ADD;
            opcodes[idx++] = 0;
            nOpcodes = idx;

            // The resulting opcodes vector would be [PUSHL, 5, PUSHV, x, ADD, 0].
            // The VM uses a [opcode, args] ABI model,
            // meaning that every instruction will be strictly an [opcode, args] pair,
            // so nOpcodes has to be an even number.
            // 'opcode' and 'args' are represented as uintptr_t internally.
        } break;
    }

    return nOpcodes;
}



static ASM_TOKEN NextToken(ASM_RESULT* res, const char** input)
{
    /*
    // Handle EOF.
    if (**input == '\0')
    {
        ASM_TOKEN t;
        t.type = TOKEN_EOF;
        return t;
    }

    // Comment parsing.
    if (**input == ';')
    {
        while (**input != '\n' && **input != '\0')
        {
            (*input)++;
        }
        if (**input == '\n')
        {
            res->line++;
            (*input)++; // Goto next line.
        }
    }

    while (isspace(**input))
    {
        if (**input == '\n') res->line++;
        (*input)++;
    }
    */

    // Avoidance loop (avoid ';' and spaces).
    while (1)
    {
        if (**input == '\0')
        {
            ASM_TOKEN t;
            _MEMSET(&t, 0, sizeof(t));
            t.type = TOKEN_EOF;
            return t;
        }

        if (isspace((unsigned char) **input))
        {
            if (**input == '\n') res->line++;
            (*input)++;
            continue;
        }

        if (**input == ';')
        {
            while (**input != '\n' && **input != '\0')
            {
                (*input)++;
            }

            continue;
        }

        break;
    }

    // Then we do the actual token parsing.
    if (**input == '"')
    {
        (*input)++;
        const char* start = *input;
        while (**input != '"' && **input != '\0')
        {
            (*input)++;
        }

        size_t length = *input - start;
        char* value = (char *) ArenaPush(res->arena, length + 1, 0);
        _MEMCPY(value, start, length);
        value[length] = '\0';

        ASM_TOKEN t;
        t.type = TOKEN_STR_LIT;
        t.textLen = length;
        _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

        // Skip closing quote.
        (*input)++;
        return t;
    }

    // Char lit.
    if (**input == '\'')
    {
        (*input)++;
        const char* start = *input;
        size_t length = 0;
        while (**input != '\'' && **input != '\0')
        {
            (*input)++;
            length++;

            if (length > 1)
            {
                ASM_TOKEN t;
                _MEMSET(&t, 0, sizeof(t));
                t.type = TOKEN_EOF;

                (*input)++;
                fprintf(stderr, "%s: Char literal has more than 1 letter!\n", __func__);
                return t;
            }
        }

        ASM_TOKEN t;
        t.type = TOKEN_CHAR_LIT;
        t.textLen = 1;
        t.text[0] = *start;
        // Skip closing quote.
        (*input)++;
        return t;
    }


    if (isalpha(**input) || **input == '_')
    {
        // Handle identifiers and keywords.
        const char* start = *input;
        while (isalnum(**input) || **input == '_')
        {
            (*input)++;
        }

        size_t length = *input - start;
        char* value = (char *) ArenaPush(res->arena, length + 1, 0);
        _MEMCPY(value, start, length);
        value[length] = '\0';

        //fprintf(stderr, "%s: Value: %.*s| length:%d\n", __func__, (int)length, value, (int)length);
        if (stringToKeyword.find(value) != stringToKeyword.end())
        {
            // We have a keyword.
            ASM_TOKEN t;
            t.type = stringToKeyword.at(value);
            t.textLen = length;
            _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

            return t;
        }
        else if (**input == ':')
        {
            // We have a label.
            ASM_TOKEN t;
            t.type = LABEL_T;
            t.textLen = length;
            _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

            (*input)++;

            return t;
        }
        else
        {
            // We have an identifier.
            ASM_TOKEN t;
            t.type = TOKEN_IDENT;
            t.textLen = length;
            _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

            return t;
        }
    }

    // Now we have to parse the values (might be registers in the future).
    if (**input == '$')
    {
        (*input)++;
        const char* start = *input;
        while (isalnum(**input))
        {
            (*input)++;
        }

        size_t length = *input - start;
        char* value = (char *) ArenaPush(res->arena, length + 1, 0);
        _MEMCPY(value, start, length);
        value[length] = '\0';

        ASM_TOKEN t;
        t.type = TOKEN_VAL;
        t.textLen = length;
        _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

        return t;
    }

    // Data regions.
    if (**input == '.')
    {
        (*input)++;

        // Handle identifiers and keywords.
        const char* start = *input;
        int colonFound = FALSE;
        while (isalnum(**input) || **input == '_' || **input == ':')
        {
            (*input)++;
            if (**input == ':')
            {
                colonFound = TRUE;
                break;
            }
        }
        if (!colonFound)
        {
            // Return error.
            ASM_TOKEN t;
            _MEMSET(&t, 0, sizeof(t));
            t.type = TOKEN_EOF;

            fprintf(stderr, "%s: Missing ':' in data region declaration.\n", __func__);
            return t;
        }

        size_t length = *input - start;
        char* value = (char *) ArenaPush(res->arena, length + 1, 0);
        _MEMCPY(value, start, length);
        value[length] = '\0';

        // We have a keyword.
        ASM_TOKEN t;
        t.type = REGION_T;
        t.textLen = length;
        _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

        (*input)++; // Skip the ':'

        return t;
    }


    if (isdigit(**input))
    {
        const char* start = *input;
        while (isalnum(**input))
        {
            (*input)++;
        }

        size_t length = *input - start;
        char* value = (char *) ArenaPush(res->arena, length + 1, 0);
        _MEMCPY(value, start, length);
        value[length] = '\0';

        ASM_TOKEN t;
        t.type = TOKEN_NUMBER_LIT;
        t.textLen = length;
        _MEMCPY(t.text, value, sizeof(u8)*t.textLen);

        return t;
    }


    ASM_TOKEN t;
    _MEMSET(&t, 0, sizeof(t));
    t.type = TOKEN_EOF;
    return t;
}

extern ASM_RESULT VM_Assemble(ANM_VM* vm, STRING8 assembly, const char* entryPoint)
{
    if (assembly.data == NULL || assembly.len == 0)
    {
        return CLITERAL(ASM_RESULT) { 0 };
    }

    ASM_RESULT res;
    _MEMSET(&res, 0, sizeof(res));
    res.valid = TRUE;
    res.arena = ArenaInit(MB(8), KB(16), ARENA_FLAG_GROWABLE);
    //res.cursor.data = assembly.data;
    res.line = 0;
    res.instrArgCounter = 0;
    res.srcVM = vm;

    // Add label table.
    res.labelTable = (ASM_LABEL_TABLE *) ArenaPush(res.arena, sizeof(ASM_LABEL_TABLE), 0);

    // A token linked list, maybe?
    ASM_TOKEN tokens[512];
    int nTokens = 0;
    ASM_TOKEN token;


    const char* input = (const char *) assembly.data;
    // Append token.
    do
    {
        token = NextToken(&res, &input);
        tokens[nTokens] = token;

        if (token.type == LABEL_T)
        {
            fprintf(stderr, "%s: We have a label!\n", __func__);

            // We have a label.
            ASM_LABEL_TABLE* table = res.labelTable;


            // NOTE: The stack can sometimes be weird.
            // Like the same address (&node) from the stack was being pushed to the linked list.
            // ASM_TOKEN_NODE node.
            ASM_TOKEN_NODE* node = (ASM_TOKEN_NODE *) ArenaPush(res.arena, sizeof(ASM_TOKEN_NODE), 0);

            node->current = &tokens[nTokens];
            node->next = NULL;
            node->nTokens = 67; // TODO: Funny number.
            SLL_APPEND_BACK(
                table->tokenChain.first,
                table->tokenChain.last, node);

            puts("DEBUG");
            fprintf(stderr, "%s: first: %p last: %p\n", __func__, table->tokenChain.first, table->tokenChain.last);

            // And then add the label.
            // (with blank offset)
            ASM_LABEL label;
            label.name = CLITERAL(STRING8) {
                (u64) node->current->textLen,
                (u8 *) node->current->text
            };
            // TODO: Addresses must be calculated from micro-operations.
            label.address = res.instrArgCounter;
            label.offset = 0x00;

            table->labels[table->nLabels++] = label;
        }

        // Handle opcode chain.
        /*
        if (IsArg(token.type) || IsOpcode(token.type))// || IsOpcode(token.type))
        {
            res.opcodeCounterEnd++;
        }
        */
        if (IsOpcode(token.type))
        {
            // Next tokens. (Do some rudimentary scanning here to determine the next tokens)
            ASM_TOKEN t;
            const char* in = input;
            int n = 0;
            do {
                t = NextToken(&res, &in);
                n++;
            } while (t.type != TOKEN_EOF && IsArg(t.type));

            ASM_TOKEN_NODE* node = (ASM_TOKEN_NODE *) ArenaPush(res.arena, sizeof(ASM_TOKEN_NODE), 0);

            node->current = &tokens[nTokens];
            node->next = NULL;
            node->nTokens = n; // Number of arguments.
            SLL_APPEND_BACK(
                res.opcodeChain.first,
                res.opcodeChain.last, node);
        }

        if ((token.type != REGION_T && IsOpcode(token.type)) || token.type == LABEL_T)
        {
            // Must have been an instruction argument.
            res.instrArgCounter++;
        }

        nTokens++;
    } while (token.type != TOKEN_EOF);

    puts("\nASSEMBLY OUTPUT:");
    fprintf(stderr, "%.*s\n", STR8_FMT(assembly));
    // Print the tokens.
    fprintf(stderr, "nTokens = %d\n", nTokens);
    for (int i = 0; i < nTokens; ++i)
    {
        ASM_TOKEN* t = &tokens[i];
        fprintf(stderr, "[%s] = %.*s\n", TypeToStr(t->type), t->textLen, t->text);
    }
    puts("");

    // Print the labels.
    {
        ASM_LABEL_TABLE* table = res.labelTable;

        puts("\nLABELS:");
        ASM_TOKEN_NODE* current = table->tokenChain.first;
        int i = 0;
        while (current != NULL)
        {
            ASM_TOKEN* t = current->current;
            fprintf(stderr, "[%s] = %.*s | address:0x%d\n", TypeToStr(t->type), t->textLen, t->text, (int) table->labels[i].address);
            current = current->next;
            i++;
        }
        puts("");
    }

    // Print the opcode chain.
    {
        puts("\nOPCODES:");

        ASM_TOKEN_NODE* current = res.opcodeChain.first;
        int i = 0;
        while (current != NULL)
        {
            ASM_TOKEN* t = current->current;
            fprintf(stderr, "[%s] = %.*s | nArgs = %d\n", TypeToStr(t->type), t->textLen, t->text, (int) current->nTokens);
            current = current->next;
            i++;
        }
        puts("");
    }

    uintptr_t bytecode[512];
    uintptr_t bytecodeCursor = 0;
    // Now form the bytecode out of fragments of the opcode chain.
    {
        puts("\nBYTECODE FORMATION:");

        ASM_TOKEN_NODE* current = res.opcodeChain.first;
        while (current != NULL)
        {
            int nOpcodes = TokensToOpcodes(&res, current->current, current->nTokens, &bytecode[bytecodeCursor]);
            current = current->next;
            bytecodeCursor += nOpcodes;
        }

        // And then print the resulting bytecode.
        fprintf(stderr, "bytecode opcode count: %d\n", (int) bytecodeCursor);
        for (int i = 0; i < (int) bytecodeCursor; i+=2)
        {
            fprintf(stderr, "%s %d ", OpcodeToStr((Opcode) bytecode[i+0]), (int) bytecode[i+1]);
        }
        puts("");
    }

    // And then we upload the bytecode format to the VM's instructions
    // TODO: we would also have to calculate the byte offset.


    // We gonna process the labels.
    /*
    {
        ASM_LABEL_TABLE* table = res.labelTable;

    }
    */


    //int nArgs;



    return res;
}

extern void VM_AssembleTerminate(ASM_RESULT *res)
{
    ArenaTerminate(res->arena);
}

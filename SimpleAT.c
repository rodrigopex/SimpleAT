#include "SimpleAT.h"

static ATCommandDescriptor *__engine;
static uint8_t __sizeOfEngine;

#if VERBOSE_MODE_ON
#include<stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define STATE_WAIT_A 0
#define STATE_WAIT_T 1
#define STATE_WAIT_PLUS 2
#define STATE_COMMAND 3
#define STATE_CHECK_COMMAND 4
#define STATE_PARAMS 5
#define STATE_STRING_PARAM 6
#define STATE_ERROR 255

#define ARRAY_LENGTH_VAR(var, condition, to) \
    { \
        for (var; condition; j += 1); \
        to = j; \
    }
#define ARRAY_LENGTH(condition, to) \
    { \
        ARRAY_LENGTH_VAR(int j = 0, condition, to) \
    }

#if ECHO_MODE_ON
#define SHOW_COMMAND()\
    for(int i = 0; i < cmdIndex; i++) {\
    __write(cmd[i]);\
    }
#else
#define SHOW_COMMAND()
#endif

/*Driver functions ---------------------*/
static uint8_t (*__open)(void);
static uint8_t (*__read)(void);
static void (*__write)(uint8_t);
static uint8_t (*__available)(void);
/*--------------------------------------*/

uint8_t ATAsciiToHex(uint8_t character)
{
    switch (character) {
    case '0': return 0x0;
    case '1': return 0x1;
    case '2': return 0x2;
    case '3': return 0x3;
    case '4': return 0x4;
    case '5': return 0x5;
    case '6': return 0x6;
    case '7': return 0x7;
    case '8': return 0x8;
    case '9': return 0x9;
    case 'a':
    case 'A': return 0xA;
    case 'b':
    case 'B': return 0xB;
    case 'c':
    case 'C': return 0xC;
    case 'd':
    case 'D': return 0xD;
    case 'e':
    case 'E': return 0xE;
    case 'f':
    case 'F': return 0xF;
    default: return 0x00;
    }
}

uint8_t ATHexToAscii(uint8_t character)
{
    switch (character) {
    case 0x0: return '0';
    case 0x1: return '1';
    case 0x2: return '2';
    case 0x3: return '3';
    case 0x4: return '4';
    case 0x5: return '5';
    case 0x6: return '6';
    case 0x7: return '7';
    case 0x8: return '8';
    case 0x9: return '9';
    case 0xA: return 'A';
    case 0xB: return 'B';
    case 0xC: return 'C';
    case 0xD: return 'D';
    case 0xE: return 'E';
    case 0xF: return 'F';
    default: return 0;
    }
}

uint8_t ATIsDigit(uint8_t character)
{
    switch (character) {
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
    case 'a':
    case 'A':
    case 'b':
    case 'B':
    case 'c':
    case 'C':
    case 'd':
    case 'D':
    case 'e':
    case 'E':
    case 'f':
    case 'F': return 1;
    default: return 0;
    }
}

#define ERROR() \
    SHOW_COMMAND()\
    __write('\n');\
    __write('\n');\
    __write('E');\
    __write('R');\
    __write('R');\
    __write('O');\
    __write('R');\
    __write('\n');

#define OK() \
    SHOW_COMMAND()\
    if (currentCommand >= 0)\
    (*__engine[currentCommand].client)(params);\
    __write('\n');\
    __write('\n');\
    __write('O');\
    __write('K');\
    __write('\n');

void __stateMachineDigest(uint8_t current)
{
    static uint8_t state;

    static int8_t currentCommand = -1;
    static uint8_t currentCommandIndex = 0;

    static uint8_t params[AT_MAX_NUMBER_OF_ARGS] = {0};
    static uint8_t currentParam;
    static uint8_t currentParamCount = 0;
    static uint8_t currentParamIndex;

    static uint8_t endOfString = 0;

#if EXTENDED_COMMANDS_ON
    static uint8_t possibleCommandsSize = 0;
    static uint8_t possibleCommands[128] = {0};
#define CLEAR_POSSIBLE_COMMANDS() \
    for (uint8_t i = 0; i < possibleCommandsSize; i += 1) possibleCommands[i] = 0; \
    possibleCommandsSize = 0;
#endif

#if ECHO_MODE_ON
    static uint8_t cmd[50] = {0};
    static uint8_t cmdIndex;
    if (state == STATE_WAIT_A)
        cmdIndex = 0;
    cmd[cmdIndex] = current;
    cmdIndex++;
#endif

    switch(state) {
    case STATE_WAIT_A:
        LOG("Wainting for A\n");
        currentCommand = -1;
        currentCommandIndex = 0;
        for (int i = 0; i < AT_MAX_NUMBER_OF_ARGS; ++i) params[i] = 0; /* Cleanning up params variables*/
        if (current == 'A')
            state = STATE_WAIT_T;
        break;
    case STATE_WAIT_T:
        LOG("Wainting for T\n");
        if (current == 'T') {
            state = STATE_WAIT_PLUS;
        } else if (current == '\n') {
            state = STATE_WAIT_A;
            ERROR();
        } else {
            state = STATE_ERROR;
        }
        break;
    case STATE_WAIT_PLUS:
        LOG("Wainting for +\n");
        if (current == '+') {
            state = STATE_COMMAND;
        } else if (current == '\n') {
            OK();
            state = STATE_WAIT_A;
        } else {
            state = STATE_ERROR;
        }
        break;
    case STATE_COMMAND:
        LOG("Wainting for command\n");
#if EXTENDED_COMMANDS_ON
        if (current == '\n') {
            state = STATE_WAIT_A;
            if (possibleCommandsSize == 1 && currentCommandIndex == __engine[possibleCommands[0]].sizeOfCommand) {
                if (__engine[possibleCommands[0]].numberOfArgs == 0) {
                    OK();
                } else {
                    ERROR();
                }
            } else {
                ERROR();
            }
            CLEAR_POSSIBLE_COMMANDS()
        } else if (possibleCommandsSize == 0) {
            for (uint8_t i = 0; i < __sizeOfEngine; ++i) {
                if (__engine[i].command[currentCommandIndex] == current) {
                    possibleCommands[possibleCommandsSize] = i;
                    possibleCommandsSize += 1;
                }
            }
            if (possibleCommandsSize == 0) {
                state = STATE_ERROR;
                CLEAR_POSSIBLE_COMMANDS()
                return;
            } else {
                currentCommandIndex += 1;
            }
        } else if (possibleCommandsSize == 1) {
            if ((currentCommandIndex < __engine[possibleCommands[0]].sizeOfCommand)
                    && (__engine[possibleCommands[0]].command[currentCommandIndex] == current)) {
                currentCommandIndex++;
            } else if (currentCommandIndex == __engine[possibleCommands[0]].sizeOfCommand) {
                if (current == '=' && __engine[possibleCommands[0]].numberOfArgs > 0) {
                    state = STATE_PARAMS;
                    currentCommand = possibleCommands[0];
                    currentParam = 0;
                    currentParamCount = 0;
                    currentParamIndex = 0;
                    possibleCommandsSize = 0;
                } else {
                    state = STATE_ERROR;
                }
                CLEAR_POSSIBLE_COMMANDS()
            } else {
                state = STATE_ERROR;
                CLEAR_POSSIBLE_COMMANDS()
            }
        } else {
            uint8_t i = 0;
            uint8_t size = possibleCommandsSize;
            possibleCommandsSize = 0;
            for (i = 0; i < size; ++i) {
                if (__engine[possibleCommands[i]].command[currentCommandIndex] == current) {
                    possibleCommands[possibleCommandsSize] = possibleCommands[i];
                    possibleCommandsSize += 1;
                }
            }
            if (possibleCommandsSize == 0) {
                state = STATE_ERROR;
                CLEAR_POSSIBLE_COMMANDS()
                return;
            } else {
                currentCommandIndex += 1;
            }
            for (i = possibleCommandsSize; i < size; ++i) {
                possibleCommands[i] = 0;
            }
        }
        break;
#else
        if (current == '\n') {
            state = STATE_WAIT_A;
            ERROR();
        } else {
            for (uint8_t i = 0; i < __sizeOfEngine; ++i) {
                if (__engine[i].command[currentCommandIndex] == current) {
                    currentCommand = (int8_t) i;
                    currentCommandIndex++;
                    state = STATE_CHECK_COMMAND;
                    return;
                }
            }
            state = STATE_ERROR; // command not found;
        }
        break;
    case STATE_CHECK_COMMAND: {
        LOG("Checking command\n");
        if (current == '\n') {
            if (currentCommandIndex == __engine[currentCommand].sizeOfCommand) {
                if (__engine[currentCommand].numberOfArgs == 0) {
                    OK();
                    state = STATE_WAIT_A;
                } else {
                    state = STATE_WAIT_A;
                    ERROR();
                }
            } else {
                state = STATE_WAIT_A;
                ERROR();
            }
        } else if ((currentCommandIndex < __engine[currentCommand].sizeOfCommand)
                   && (__engine[currentCommand].command[currentCommandIndex] == current)) {
            currentCommandIndex++;
        } else if (currentCommandIndex == __engine[currentCommand].sizeOfCommand) {
            if (current == '=' && __engine[currentCommand].numberOfArgs > 0) {
                state = STATE_PARAMS;
                currentParam = 0;
                currentParamCount = 0;
                currentParamIndex = 0;
            } else {
                state = STATE_ERROR;
            }
        } else {
            state = STATE_ERROR;
        }
        break;
    }
#endif
    case STATE_PARAMS: {
        LOG("Getting params\n"); // get paramenters
        uint8_t sizeInBytes = (uint8_t) (__engine[currentCommand].argsSize[currentParam] << 1);
        //uint8_t sizeOfParameter = (uint8_t) __engine[currentCommand].argsSize[currentParam];
        LOG("currentParam %d, sizeInBytes %d Current %d\n", currentParam, sizeInBytes, current);
        if (current == '\n') {
            if (currentParamIndex == sizeInBytes
                    && (__engine[currentCommand].numberOfArgs == currentParam + 1)) {
                OK();
                state = STATE_WAIT_A;
            } else {
                state = STATE_WAIT_A;
                ERROR();
            }
        } else if (current == '"') {
            if (currentParamIndex == 0) {
                //begin of string skip
                endOfString = 0;
                state = STATE_STRING_PARAM;
            } else {
                state = STATE_ERROR;
            }
        } else if (ATIsDigit(current) && currentParamIndex < sizeInBytes) {
            params[currentParamCount] |= ATAsciiToHex(current) << (4 * (1 - (currentParamIndex % 2)));
            currentParamCount += currentParamIndex % 2;
            currentParamIndex++;
            LOG("Param %d, value %d, at %d\n", params[currentParamCount], ATAsciiToHex(current) << (4 * (1 - (currentParamIndex % 2))), currentParamCount);
        } else if (currentParamIndex == sizeInBytes) {
            if (__engine[currentCommand].numberOfArgs > currentParam + 1) {
                if (current == ',') {
                    currentParamIndex = 0;
                    currentParam++;
                } else {
                    state = STATE_ERROR;
                }
            } else {
                state = STATE_ERROR;
            }
        } else {
            state = STATE_ERROR;
        }
        break;
    }
    case STATE_STRING_PARAM: {
        LOG("Getting string param\n");
        if (current == '\n') {
            if (currentParamIndex == __engine[currentCommand].argsSize[currentParam]
                    && (__engine[currentCommand].numberOfArgs == 1)) {
                state = STATE_WAIT_A;
                OK();
            } else {
                LOG("ERROR1\n");
                state = STATE_WAIT_A;
                ERROR();
            }
        } else if (current == '"') {
            if (currentParamIndex < AT_MAX_SIZE_STRING) {
                //end of string skip and set size of it.
                __engine[currentCommand].argsSize[currentParam] = (int8_t)currentParamIndex;
                endOfString = 1;
                LOG("End of string\n");
            } else {
                LOG("ERROR2\n");
                state = STATE_ERROR;
            }
        } else if (current < 128 && currentParamIndex < AT_MAX_SIZE_STRING) {
            if (endOfString) {
                LOG("ERROR3\n");
                state = STATE_ERROR;
            } else {
                params[currentParamIndex] = current;
                currentParamIndex++;
                LOG("Param %c\n", (char) current);
            }
        }  else {
            LOG("ERROR4\n");
            state = STATE_ERROR;
        }
        break;
    }
    case STATE_ERROR:
        LOG("State cleanning...\n");
        if (current == '\n') { //cleaning input...
            state = STATE_WAIT_A;
            ERROR();
        }
        break;
    default:
        ERROR();
    }
}

void ATEngineDriverInit(uint8_t (*open)(void), uint8_t (*read)(void), void (*write)(uint8_t),
                        uint8_t (*available)(void))
{
    __open = open;
    __read = read;
    __write = write;
    __available = available;
}

void ATEngineInit(ATCommandDescriptor *engine, uint8_t sizeOfEngine)
{
    __engine = engine;

    if (sizeOfEngine < 128) __sizeOfEngine = sizeOfEngine;
    else __sizeOfEngine = 128;

    for (int i = 0; i < __sizeOfEngine; ++i) {
        uint8_t j;
        ARRAY_LENGTH_VAR(j = 0, __engine[i].command[j] != '\0', __engine[i].sizeOfCommand)
        ARRAY_LENGTH_VAR(j = 0, __engine[i].argsSize[j] > 0, __engine[i].numberOfArgs)
    }
    __open();
}

// Used for polling way to do
uint8_t ATEnginePollingHandle()
{
    while(__available()) {
        __stateMachineDigest(__read());
    }
    return 1;
}

void ATEngineInterruptHandle(uint8_t data)
{ /* Used for interrupt way to do*/
    __stateMachineDigest(data);
}

void ATReplyWithByte(uint8_t data)
{
    __write(ATHexToAscii((data & 0xF0) >> 4));
    __write(ATHexToAscii(data & 0x0F));
}

void ATReplyWithByteArray(uint8_t *msg, int size)
{
    for (int i = size - 1; i >=0; --i) {
        ATReplyWithByte(msg[i]);
    }
}

void ATReplyWithString(char *str)
{
    for (int i = 0; str[i] != '\0'; ++i) __write((uint8_t)str[i]);
}

void ATReplyWithChar(char c)
{
    __write(c);
}

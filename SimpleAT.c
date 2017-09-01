#include "SimpleAT.h"

static ATCommandDescriptor *__engine;
static uint8_t __sizeOfEngine;

#if 0
#define LOG(x) printf(x)
#else
#define LOG(x)
#endif

/*Driver functions ---------------------*/
static uint8_t (*__open)(void);
static uint8_t (*__read)(void);
static void (*__write)(uint8_t);
static uint8_t (*__available)(void);
/*--------------------------------------*/

#define SHOW_COMMAND() for(int i = 0; i < cmdIndex; i++) {printf("%c",cmd[i])}

uint8_t asciiToUint8(uint8_t character) {
    switch (character) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
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

uint8_t isDigit(uint8_t character) {
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
    for(int i = 0; i < cmdIndex; i++) {\
        __write(cmd[i]);\
    }\
    __write('\n');\
    __write('\n');\
    __write('E');\
    __write('R');\
    __write('R');\
    __write('O');\
    __write('R');\
    __write('\n');


#define OK() \
    for(int i = 0; i < cmdIndex; i++) {\
        __write(cmd[i]);\
    }\
    if(currentCommand >= 0)\
        (*__engine[currentCommand].client)(params);\
    __write('\n');\
    __write('\n');\
    __write('O');\
    __write('K');\
    __write('\n');

void __stateMachineDigest(uint8_t current) {
    static uint8_t state;

    static int8_t currentCommand = -1;
    static uint8_t currentCommandIndex = 0;

    static uint16_t params[AT_MAX_NUMBER_OF_ARGS] = {0};
    static uint8_t currentParam;
    static uint8_t currentParamIndex;

    static uint8_t cmd[50] = {0};
    static uint8_t cmdIndex;
    if(state == 0)
        cmdIndex = 0;
    cmd[cmdIndex] = current;
    cmdIndex++;

    switch(state) {
    case 0:
        LOG("State 0\n");
        currentCommand = -1;
        currentCommandIndex = 0;
        if(current == 'A')
            state = 1;
        break;
    case 1:
        LOG("State 1\n");
        if(current == 'T')
            state = 2;
        else if(current == '\n') {
            state = 0;
            ERROR();
        } else
            state = 255; //error
        break;
    case 2:
        LOG("State 2\n");
        if(current == '+'){
            state = 3;
        } else if(current == '\n') {
            OK();
            state = 0;
        } else
            state = 255; //error
        break;
    case 3:
        LOG("State 3\n");
        if(current == '\n') {
            state = 0;
            ERROR();
        } else {
            for(uint8_t i = 0; i < __sizeOfEngine; ++i) {
                if(__engine[i].command[currentCommandIndex] == current) {
                    currentCommand = (int8_t) i;
                    currentCommandIndex++;
                    state = 4;
                    return;
                }
            }
            state = 255; // error No command found;
        }
        break;
    case 4: {
        LOG("State 4\n");
        if(current == '\n') {
            if(currentCommandIndex == __engine[currentCommand].sizeOfCommand) {
                if(__engine[currentCommand].numberOfArgs == 0) {
                    OK();
                    state = 0;
                } else {
                    state = 0;
                    ERROR();
                }
            } else {
                state = 0;
                ERROR();
            }
        } else if((currentCommandIndex < __engine[currentCommand].sizeOfCommand) &&
                  (__engine[currentCommand].command[currentCommandIndex] == current)) {
            currentCommandIndex++;
        } else if(currentCommandIndex == __engine[currentCommand].sizeOfCommand) {

            if(current == '=' && __engine[currentCommand].numberOfArgs > 0){
                state = 5;
                currentParam = 0;
                currentParamIndex = 0;
            } else {
                state = 255;
            }
        } else {
            state = 255;
        }
        break;
    }
    case 5:
        LOG("State 5\n"); // get paramenters
        uint8_t sizeInBytes = (uint8_t) __engine[currentCommand].argsSize[currentParam]<<1;
        if(current == '\n') {
            if(currentParamIndex == sizeInBytes && (__engine[currentCommand].numberOfArgs == currentParam + 1)) {
                OK();
                state = 0;
            } else {
                state = 0;
                ERROR();
            }
        } else if(isDigit(current) && currentParamIndex < sizeInBytes) {
            params[currentParam] |= (uint32_t) asciiToUint8(current) << (4 * (sizeInBytes - currentParamIndex - 1));
            currentParamIndex++;
        } else if(currentParamIndex == sizeInBytes) {
            if(__engine[currentCommand].numberOfArgs > currentParam + 1) {
                if(current == ','){
                    currentParamIndex = 0;
                    currentParam++;
                } else {
                    state = 255;
                }
            } else {
                state = 255;
            }
        }else
            state = 255; //error
        break;
    case 255:
        if(current == '\n') { //cleaning input...
            state = 0;
            ERROR();
        }
        break;
    default:
        ERROR();
    }

}

void ATEngineDriverInit(uint8_t (*open)(void),
                          uint8_t (*read)(void),
                          void (*write)(uint8_t),
                          uint8_t (*available)(void)) {
    __open = open;
    __read = read;
    __write = write;
    __available = available;
}

void ATEngineInit(ATCommandDescriptor *engine, uint8_t sizeOfEngine) {
    __engine = engine;
    for(int i = 0; i < sizeOfEngine; ++i) {
        ATReplyWithString("Init size calculation\n");
        int j;
        for(j = 0; __engine[i].command[j] != '\0'; ++j);
        __engine[i].sizeOfCommand = (uint8_t)j;
        for(j = 0; __engine[i].argsSize[j] > 0; ++j);
        __engine[i].numberOfArgs = (uint8_t)j;
        ATReplyWithString("Init size calculation\n");
    }
    __sizeOfEngine = sizeOfEngine;
    __open();
}

uint8_t ATEngineRun() {
    while(__available()) {
        __stateMachineDigest(__read());
    }
    return 1;
}

void ATReply(uint8_t *msg, int size)
{
    for(int i = 0; msg[i] < size; ++i) __write(msg[i]);
}

void ATReplyWithString(char *str)
{
    for(int i = 0; str[i] != '\0'; ++i) __write((uint8_t)str[i]);
}

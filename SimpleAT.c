#include "SimpleAT.h"

static ATCommandDescriptor *__engine;
static uint8_t __sizeOfEngine;

#if VERBOSE_MODE_ON
#include<stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

#define MAX_COMMAND_BASE_STRING_SIZE 50
#define MAX_COMMAND_SIZE 250
#define END_OF_COMMAND '!'

typedef enum {
    kInit,
    kReading,
    kParsing,
    kCallingClient,
    kReportError
} ATEngineState;


char *AYCommandGetBaseString(AYCommand *cmd) {
    char *baseStringRef = 0;
    if(cmd->commandString[2] == '+') {
        baseStringRef = cmd->commandString + 3;
    }
    return baseStringRef;
}

char *AYCommandGetArgAtIndex(AYCommand *cmd, uint16_t index)
{
    //    LOG(__FUNCTION__);
    char *baseStringRef = 0;
    if(index < cmd->numberOfArgs) {
        int8_t count = -1;
        for(char *cursor = cmd->commandString ; *cursor != END_OF_COMMAND; cursor++) {
            if(*cursor == 0) {
                count++;
                if(count == index) {
                    if(*(cursor+1) != END_OF_COMMAND)
                        baseStringRef = cursor + 1;
                    break;
                }
            }
        }
    }
    return baseStringRef;
}


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

void __stateMachineDigest(uint8_t current) {
    static uint8_t state = 0;
    static char commandBuffer[MAX_COMMAND_SIZE];
    static char *commandBufferCursor;
    static char *commandBufferEnd = commandBuffer + MAX_COMMAND_SIZE - 1;
    static AYCommand command = {commandBuffer, 0};

    switch (state) {
    case kInit:
        commandBufferCursor = commandBuffer;
        for(uint16_t i = 0; i < MAX_COMMAND_SIZE; ++i) commandBuffer[i] = 0;
        state = kReading;
    case kReading:
        if(current != END_OF_COMMAND && commandBufferCursor < commandBufferEnd) {
            *commandBufferCursor = (char) current;
            ++commandBufferCursor;
            break;
        }
        *commandBufferCursor = END_OF_COMMAND;
    case kParsing:
        AYCommandDigest(&command);
#ifdef ECHO_MODE_ON
        ATReplyWithString(command.commandString);
        if(command.numberOfArgs > 0) {
            ATReplyWithString("=");
            for(int i = 0; i < command.numberOfArgs; ++i) {
                if(i > 0) ATReplyWithString(",");
                ATReplyWithString(AYCommandGetArgAtIndex(&command, i));
            }
        }
        ATReplyWithString("\n");
#endif
        ATCommandDescriptor *currentCommand = 0;
        for(int i = 0; __engine[i].client != 0; ++i) {
            //LOG("%d, %d\n", command.numberOfArgs, __engine[i].numberOfArgs);
            //LOG("%s, %s\n", AYCommandGetBaseString(&command), __engine[i].command);
            if(command.numberOfArgs == __engine[i].numberOfArgs
                    && AYStringCompare(AYCommandGetBaseString(&command), __engine[i].command)) {
                currentCommand = &__engine[i];
                break;

            }
        }
        if(currentCommand) {
            (currentCommand->client)(&command);
            ATReplyWithString("\n\nOK\n");
        } else {
            if(AYStringCompare(command.commandString, "AT")) {
                ATReplyWithString("\n\nOK\n");
            } else {
                ATReplyWithString("\n\nERROR\n");
            }
        }
        state = kInit;
    default:
        //TODO: check the unknown state
        break;
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

void ATEngineInit(ATCommandDescriptor *engine)
{
    __engine = engine;
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

void ATReplyWithNumberWithSize(uint64_t number, int size) {
    ATReplyWithByteArray((uint8_t *) &number, size);
}

void ATReplyWithString(char *str)
{
    if(str) {
        for (uint16_t i = 0; str[i] != '\0'; ++i) __write((uint8_t)str[i]);
    }
}

void ATReplyWithChar(char c)
{
    __write(c);
}


uint8_t AYStringCompare(char *str1, char *str2) {
    //    LOG(__FUNCTION__);
    uint8_t result = 0;
    if(str1 && str2) {
        for(char *cStr1 = str1, *cStr2 = str2; *cStr1 == *cStr2; cStr1++, cStr2++) {
            if(*cStr1 == 0) {
                result = 1;
                break;
            }
        }
    } else if(str1 == 0 && str2 == 0) {
        result = 1;
    }
    return result;
}

void AYCommandDigest(AYCommand *aDesc)
{
    //    LOG(__FUNCTION__);
    aDesc->numberOfArgs = 0;
    char *cursor = aDesc->commandString;
    uint16_t sizeOfArg = 0;
    for(; *cursor != END_OF_COMMAND; ++cursor, ++sizeOfArg) {
        if(*cursor == '=' || *cursor == ',') {
            *cursor = 0;
            ++aDesc->numberOfArgs;
        }
    }
    *cursor = 0; /* replaces the end of line at the end*/
}

uint16_t AYStringLength(char *str) {
    //    LOG(__FUNCTION__);
    uint16_t length = 0;
    if(str) {
        for(; str[length] != 0; ++length) {}
        if(length >= 1) --length;
    }
    return length;
}

uint64_t AYStringToNumber(char *str)
{
    //    LOG(__FUNCTION__);
    uint64_t result = 0;
    int strLen = AYStringLength(str);
    if(strLen) {
        for(int i = strLen; i >= 0; --i) {
            result += ATAsciiToHex(str[i]) << ((strLen - i) << 2);
        }
    }
    return result;
}

uint16_t AYCommandGetNumberOfArgs(AYCommand *cmd)
{
    return cmd->numberOfArgs;
}

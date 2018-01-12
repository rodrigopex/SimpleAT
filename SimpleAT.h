#ifndef SIMPLEAT_H
#define SIMPLEAT_H
#include <stdint.h>

#define ECHO_MODE_ON 1
#define VERBOSE_MODE_ON 1

#define AT_COMMAND(name, args, client) {(char*)#name, args, client}
#define AT_END_OF_COMMANDS {"AT", 0, 0}

//#define ATReplyByteArray(x) ((uint8_t *) &x), sizeof(x)

typedef struct {
    char *commandString;
    uint16_t numberOfArgs;
} AYCommand;

typedef struct {
    char *command;
    uint8_t numberOfArgs;
    void (*client)(AYCommand*);
} ATCommandDescriptor;

void ATEngineDriverInit(uint8_t (*open)(void), uint8_t (*read)(void), void (*write)(uint8_t),
                        uint8_t (*available)(void));
void ATEngineInit(ATCommandDescriptor *engine);
uint8_t ATEnginePollingHandle();
void ATEngineInterruptHandle(uint8_t data);

void AYCommandDigest(AYCommand *aDesc);
char *AYCommandGetArgAtIndex(AYCommand *cmd, uint16_t index);
char *AYCommandGetBaseString(AYCommand *cmd);
uint16_t AYCommandGetNumberOfArgs(AYCommand *cmd);

void ATReplyWithByteArray(uint8_t *data, int size);
void ATReplyWithByte(uint8_t data);
void ATReplyWithString(char *str);
void ATReplyWithChar(char c);
void ATReplyWithNumberWithSize(uint64_t number, int size);
#define ATReplyWithNumber(x) ATReplyWithNumberWithSize(x, sizeof(x));

uint8_t AYStringCompare(char *str1, char *str2);
uint64_t AYStringToNumber(char *str);
uint16_t AYStringLength(char *str);

#endif // SIMPLEAT_H

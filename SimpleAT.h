#ifndef SIMPLEAT_H
#define SIMPLEAT_H
#include <stdint.h>

#define AT_MAX_NUMBER_OF_ARGS 40 /*In bytes*/
#define AT_MAX_SIZE_STRING 20
#define CLIENT_FUNCTION_TYPE uint8_t
#define ECHO_MODE_ON 1
#define VERBOSE_MODE_ON 0
#define EXTENDED_COMMANDS_ON 0

#define AT_NO_ARGS {0}
#define AT_TYPE_STRING AT_MAX_SIZE_STRING
#define AT_ARGS(...) {__VA_ARGS__, -1}
#define AT_TYPE(x) ((uint8_t) sizeof (x))
#define AT_COMMAND(name, args, client) {(char*)#name, args, client}

#define ATReplyByteArray(x) ((uint8_t *) &x), sizeof(x)

typedef struct {
    char *command;
    uint8_t numberOfArgs;
    void (*client)(const uint8_t*);
} ATCommandDescriptor;

typedef struct {
    char *commandString;
    uint8_t numberOfArgs;
} AYCommand;


void ATEngineDriverInit(uint8_t (*open)(void), uint8_t (*read)(void), void (*write)(uint8_t),
                        uint8_t (*available)(void));

void ATEngineInit(ATCommandDescriptor *engine);
uint8_t ATEnginePollingHandle();

void AYCommandDigest(AYCommand *aDesc);

void ATEngineInterruptHandle(uint8_t data);
void ATReplyWithByteArray(uint8_t *data, int size);
void ATReplyWithByte(uint8_t data);
void ATReplyWithString(char *str);
void ATReplyWithChar(char c);

#endif // SIMPLEAT_H

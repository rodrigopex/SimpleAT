#ifndef SIMPLEAT_H
#define SIMPLEAT_H
#include <stdint.h>

#define AT_MAX_NUMBER_OF_ARGS 4
#define CLIENT_FUNCTION_TYPE uint16_t

#define AT_NO_ARGS {0}
#define AT_ARGS(...) {__VA_ARGS__, -1}
#define AT_ARG(x) ((uint8_t) sizeof (x))
#define AT_COMMAND(name, args, client) {#name, 0, args , 0, client}


typedef struct {
    char *command;
    uint8_t sizeOfCommand;
    int8_t argsSize[AT_MAX_NUMBER_OF_ARGS];
    uint8_t numberOfArgs;
    void (*client)(const CLIENT_FUNCTION_TYPE*);
} ATCommandDescriptor;

void ATEngineDriverInit(uint8_t (*open)(void),
                        uint8_t (*read)(void),
                        void (*write)(uint8_t),
                        uint8_t (*available)(void));

void ATEngineInit(ATCommandDescriptor *engine,  uint8_t sizeOfEngine);
uint8_t ATEngineRun();
void ATReplyWithByteArray(uint8_t *msg, int size);
void ATReplyWithString(char *str);



#endif // SIMPLEAT_H

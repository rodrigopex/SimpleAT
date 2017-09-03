# SimpleAT - 0.2
Simple API to provide AT commands to embedded systems

To use this lib you must to do:
* Download the SimpleAT.h and SimpleAT.c to your source code;
* Implement the driver to read, write, check availability and open the UART module at your system;
* Initialize the driver using the ATEngineDriverInit function passing the above mentioned functions related to the UART;
* Define the command list as the below example. There, you must define the parameters (your function must receive from the command the types must be uintX_t, x can be 8, 16, 32 and 64) and callback function that must be of type as shown at the example (```void startClient(const uint16_t *args)```)
* Initialize the AT engine passing the pointer to the command list the its size to the function ATEngineInit;
* Run it in a nonblocking way by using the function ATEngineRun() that always returns true. It can be used at the condition of the while true or inside it.

Example of the use of the lib in a regular PC using printf instead of using UART for display character at the command line.
```C
#include "SimpleAT.h"

static int __currentData = 0;
static char *__data;

int StubInit(int argc, char *argv[]) {
    if(argc == 1) {
        printf("No commands...\n");
        return 0;
    } else {
        printf("data -> [%s]\n",argv[1]);
    }
    __data = argv[1];
    __currentData = 0;
    return 1;
}

uint8_t StubOpen() {
    return 0;
}

void StubWrite(uint8_t data) {
    printf("%c", (char) data);
}

uint8_t StubAvailable() {

    return (uint8_t) __data[__currentData] != '\0';
}

uint8_t StubRead() {
    uint8_t readData = 0xFF;
    if(StubAvailable()){
         readData = (uint8_t) __data[__currentData];
        __currentData++;
    }
    return readData;
}

void startClient(const uint16_t *args){
    (void) args;
    ATReplyWithString((char*)"Results of ");
    ATReplyWithString((char*)__FUNCTION__);
}

void readClient(const uint16_t *args){
    uint16_t addr = args[0];
    ATReplyWithString((char*) "Results of ");
	ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithByteArray(ATReplyByteArray(addr));
}

void writeClient(const uint16_t *args){
    uint16_t addr = args[0];
    uint8_t value = (uint8_t) args[1];
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithByteArray(ATReplyByteArray(addr));
    ATReplyWithString((char*) " VALUE: ");
    ATReplyWithByte(value);
}

int main(int argc, char **argv) {

    StubInit(argc, argv);

    ATEngineDriverInit(StubOpen,
                       StubRead,
                       StubWrite,
                       StubAvailable);

    ATCommandDescriptor atEngine[] = {
        AT_COMMAND(START, AT_NO_ARGS, startClient),
        AT_COMMAND(READ, AT_ARGS(AT_TYPE(uint16_t)), readClient),
        AT_COMMAND(WRITE, AT_ARGS(AT_TYPE(uint16_t),AT_TYPE(uint8_t)), writeClient)
    };
    ATEngineInit(atEngine, 3);
    while(ATEngineRun()) {
        //spare code
    }
    return 0;

}
```
## Using the interface
### Basic test, no arguments and no command:
```
AT


OK
```
### No arguments:
```
AT+START
Starting chip...

OK
```
### One argument:
```
AT+READ=FA06
REG[FA06] = 0xF1

OK
```
### One argument:
```
AT+WRITE=265C,AF


OK
```

## Knwon limitations

* The commands cannot begin with the same letter;
* You can only have up to 256 commands;


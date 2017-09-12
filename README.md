# SimpleAT - 0.2
Simple API to provide AT commands to embedded systems

To use this lib you must to do:
* Download the SimpleAT.h and SimpleAT.c to your source code;
* Implement the driver to read, write, check availability and open the UART module at your system;
* Initialize the driver using the ATEngineDriverInit function passing the above mentioned functions related to the UART;
* Define the command list as the below example. There, you must define the parameters (your function must receive from the command the types must be uintX_t, x can be 8, 16, 32 and 64) and callback function that must be of type as shown at the example (```void startClient(const uint8_t *args)```)
* Initialize the AT engine passing the pointer to the command list the its size to the function ATEngineInit;
* Run it in a polling nonblocking way by using the function ATEnginePollingRun() that always returns true. It can be used at the condition of the while true or inside it. Or using interrupt when there is data available at the uart buffer you can call ATEngineInterruptHandle(...) instead of using the pooling one.

There is a project that is used to test the lib. For more details follow the link https://github.com/rodrigopex/SimpleAT_test.

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

void startClient(const uint8_t *args){
    (void) args;
    ATReplyWithString((char*)"Results of ");
    ATReplyWithString((char*)__FUNCTION__);
}

void readClient(const uint8_t *args){
    uint16_t addr = (uint16_t)((args[0] << 8) + args[1]);
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithByteArray(ATReplyByteArray(addr));
}

void setClient(const uint8_t *args){
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " STR: ");
    ATReplyWithString((char *) args);
}

void writeClient(const uint8_t *args){
    uint16_t addr = (uint16_t)((args[0] << 8) + args[1]);
    uint8_t value = args[1];
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithByteArray(ATReplyByteArray(addr));
    ATReplyWithString((char*) " VALUE: ");
    ATReplyWithByte(value);
}

int main(int argc, char **argv) {
    /* Commando to test the lib
    * ./build/SimpleATTest "$(cat test.cmd)" > test.log && diff test.log test.log_ok
    * If there is difference there error.
    */
    if(StubInit(argc, argv)) return 1;

    ATEngineDriverInit(StubOpen,
                       StubRead,
                       StubWrite,
                       StubAvailable);

    ATCommandDescriptor atEngine[] = {
        AT_COMMAND(START, AT_NO_ARGS, startClient),
        AT_COMMAND(READ, AT_ARGS(AT_TYPE(uint16_t)), readClient),
        AT_COMMAND(CHANGE, AT_ARGS(AT_TYPE_STRING), setClient),
        AT_COMMAND(WRITE, AT_ARGS(AT_TYPE(uint16_t), AT_TYPE(uint8_t)), writeClient)
    };
    ATEngineInit(atEngine, 4);
    while(ATEnginePollingHandle()) {
        //spare code
        return 0;
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
### Two argument:
```
AT+WRITE=265C,AF


OK
```

### String:
```
AT+CHANGE="Hello World!"


OK
```

## Knwon limitations

* The commands cannot begin with the same letter;
* You can only have up to 256 commands;
* String command can only have one argument.


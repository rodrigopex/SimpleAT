# SimpleAT - 0.2
Simple API to provide AT commands to embedded systems

To use this lib you must to do:
* Download the SimpleAT.h and SimpleAT.c to your source code;
* Implement the driver to read, write, check availability and open the UART module at your system;
* Initialize the driver using the ATEngineDriverInit function passing the above mentioned functions related to the UART;

```C
uint8_t open();

void write(uint8_t data);

uint8_t available();

uint8_t read();
```


* Define the command list as the below example.
* Initialize the AT engine passing the pointer to the command list the its size to the function ATEngineInit;
* Run it in a polling nonblocking way by using the function ATEnginePollingRun() that always returns true. It can be used at the condition of the while true or inside it. Or using interrupt when there is data available at the uart buffer you can call ATEngineInterruptHandle(...) instead of using the pooling one.

There is a project that is used to test the lib. For more details follow the link https://github.com/rodrigopex/SimpleAT_test.

Example of the use of the lib in a regular PC using printf instead of using UART for display character at the command line.
```C
#include "SimpleAT.h"
#include "Stub.h"

void testClient(AYCommand *cmd)
{
    ATReplyWithString("Number or arguments: ");
    ATReplyWithNumber(AYCommandGetNumberOfArgs(cmd));
}

void startClient(AYCommand *args)
{
    (void) args;
    ATReplyWithString((char*)"Results of ");
    ATReplyWithString((char*)__FUNCTION__);
}

void readClient(AYCommand *cmd)
{
    uint16_t addr = (uint16_t) AYStringToNumber(AYCommandGetArgAtIndex(cmd, 0));
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithNumber(addr);
}

void setClient(AYCommand *cmd)
{
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " STR: ");
    ATReplyWithString((char *) AYCommandGetArgAtIndex(cmd, 0));
}

void writeClient(AYCommand *cmd)
{
    uint16_t addr = AYStringToNumber(AYCommandGetArgAtIndex(cmd, 0));
    uint8_t value = AYStringToNumber(AYCommandGetArgAtIndex(cmd, 1));
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithNumber(addr);
    ATReplyWithString((char*) " VALUE: ");
    ATReplyWithNumber(value);
}

int main(int argc, char **argv) {
    if(StubInit(argc, argv)) return 1;

    ATEngineDriverInit(StubOpen,
                       StubRead,
                       StubWrite,
                       StubAvailable);

    ATCommandDescriptor atEngine[] = {
        AT_COMMAND(START, 0, startClient),
        AT_COMMAND(READ, 1, readClient),
        AT_COMMAND(CHANGE, 1, setClient),
        AT_COMMAND(WRITE, 2, writeClient),
        AT_COMMAND(TEST, 4, testClient),
        AT_END_OF_COMMANDS
    };
    ATEngineInit(atEngine);
    while(ATEnginePollingHandle()) {
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
REG[265C] = 0xAF

OK
```

### String:
```
AT+CHANGE=Hello World!
Hello world!

OK
```

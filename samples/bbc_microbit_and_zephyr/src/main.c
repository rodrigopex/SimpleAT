/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <misc/printk.h>
#include <uart.h>

#include "../../../SimpleAT.h"

static void uart_fifo_callback(struct device *dev)
{
    u8_t recvData;
    /* Verify uart_irq_update() */
    if (!uart_irq_update(dev)) {
        printk("retval should always be 1\n");
        return;
    }
    /* Verify uart_irq_rx_ready() */
    if (uart_irq_rx_ready(dev)) {
        /* Verify uart_fifo_read() */
        uart_fifo_read(dev, &recvData, 1);
        ATEngineInterruptHandle(recvData);
    }
}

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
    ATReplyWithString(AYCommandGetArgAtIndex(cmd, 0));
    uint16_t addr = (uint16_t) AYHexStringToNumber(AYCommandGetArgAtIndex(cmd, 0));
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
    uint16_t addr = AYHexStringToNumber(AYCommandGetArgAtIndex(cmd, 0));
    uint8_t value = AYHexStringToNumber(AYCommandGetArgAtIndex(cmd, 1));
    ATReplyWithString((char*) "Results of ");
    ATReplyWithString((char*)__FUNCTION__);
    ATReplyWithString((char*) " ADDR: ");
    ATReplyWithNumber(addr);
    ATReplyWithString((char*) " VALUE: ");
    ATReplyWithNumber(value);
}

uint8_t driverOpen() {
    struct device *uart_dev = device_get_binding("UART_0");
    if(!uart_dev) {
    printk("Problem to load uart device\n");
        return 1;
    }
    printk("UART device loaded successfully!\n");

    /* Verify uart_irq_callback_set() */
    uart_irq_callback_set(uart_dev, uart_fifo_callback);

    /* Enable Tx/Rx interrupt before using fifo */
    /* Verify uart_irq_rx_enable() */
    uart_irq_rx_enable(uart_dev);
    printk("Now you can start sending AT commands\n");
    return 0;
}

void driverWrite(uint8_t data) {
    printk("%c", (char) data);
}

void main(void)
{
    printk("Version 0.1, %s", CONFIG_ARCH);
    ATEngineDriverInit(driverOpen,
                       NULL,
                       driverWrite,
                       NULL);
    static const ATCommandDescriptor atEngine[] = {
        AT_COMMAND(START, 0, startClient),
        AT_COMMAND(READ, 1, readClient),
        AT_COMMAND(CHANGE, 1, setClient),
        AT_COMMAND(WRITE, 2, writeClient),
        AT_COMMAND(TEST, 4, testClient),
        AT_END_OF_COMMANDS
    };
    ATEngineInit(atEngine);
}

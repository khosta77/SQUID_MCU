#include "motor_controller.hpp"
#include "motor_driver.hpp"
#include "../system/include/cmsis/stm32f4xx.h"
#include <cstring>

static void handleVersionCommand();
static void handleStatusCommand();
static void handleStopCommand();
static void handleSyncMoveCommand(const uint8_t* data, uint16_t dataLen);
static void handleAsyncMoveCommand(const uint8_t* data, uint16_t dataLen);

void processPacketCommand(const PacketParser& packet) {
    uint8_t cmd = packet.getCommand();
    const uint8_t* data = packet.getData();
    uint16_t dataLen = packet.getDataLength();

    switch (cmd) {
        case Cmd::VERSION:
            handleVersionCommand();
            break;

        case Cmd::STATUS:
            handleStatusCommand();
            break;

        case Cmd::STOP:
            handleStopCommand();
            break;

        case Cmd::SYNC_MOVE:
            handleSyncMoveCommand(data, dataLen);
            break;

        case Cmd::ASYNC_MOVE:
            handleAsyncMoveCommand(data, dataLen);
            break;

        default:
            sendErrorPacket(Error::INVALID_COMMAND);
            break;
    }
}

static void handleVersionCommand() {
    sendVersionResponse();
}

static void handleStatusCommand() {
    uint16_t statusPins = GPIOE->IDR & 0x03FF;
    sendStatusResponse(g_motorDriver.getActiveMotors(), g_motorDriver.getCompletedMotors(), statusPins);
}

static void handleStopCommand() {
    g_motorDriver.stopAll();
    sendStopResponse(Result::SUCCESS);
}

static void handleSyncMoveCommand(const uint8_t* data, uint16_t dataLen) {
    if (dataLen == 0 || dataLen % 16 != 0) {
        sendErrorPacket(Error::INVALID_MOTOR_COUNT);
        return;
    }

    uint8_t motorCount = dataLen / 16;
    if (motorCount > MAX_MOTORS) {
        sendErrorPacket(Error::INVALID_MOTOR_COUNT);
        return;
    }

    g_motorDriver.startMotors(data, motorCount);
    while (!g_motorDriver.allComplete()) {
        __WFI();
    }

    sendMoveResponse(Result::SUCCESS);
}

static void handleAsyncMoveCommand(const uint8_t* data, uint16_t dataLen) {
    if (dataLen == 0 || dataLen % 16 != 0) {
        sendErrorPacket(Error::INVALID_MOTOR_COUNT);
        return;
    }

    uint8_t motorCount = dataLen / 16;
    if (motorCount > MAX_MOTORS) {
        sendErrorPacket(Error::INVALID_MOTOR_COUNT);
        return;
    }

    g_motorDriver.startMotors(data, motorCount);
    sendMoveResponse(Result::SUCCESS);
}

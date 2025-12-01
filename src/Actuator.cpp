#include "Actuator.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

Actuator::Actuator(int pinOutOmhoog, int pinOutOmlaag, int pinInOmhoog, int pinInOmlaag)
    : mBrug(false),
      mPinOutOmhoog(pinOutOmhoog),
      mPinOutOmlaag(pinOutOmlaag),
      mPinInOmhoog(pinInOmhoog),
      mPinInOmlaag(pinInOmlaag),
      mTask(nullptr),
      mBridgeQueue(nullptr),
      mCommand(Command::NONE)
{
    // Configure output pins
    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << mPinOutOmhoog) | (1ULL << mPinOutOmlaag);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);

    // Configure input pins if provided
    if (mPinInOmhoog >= 0 && mPinInOmlaag >= 0) {
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << mPinInOmhoog) | (1ULL << mPinInOmlaag);
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
    }

    std::printf("[Actuator] Geïnitialiseerd: OutOmhoog=%d OutOmlaag=%d InOmhoog=%d InOmlaag=%d\n",
                mPinOutOmhoog, mPinOutOmlaag, mPinInOmhoog, mPinInOmlaag);
}

void Actuator::sendEvent(BridgeEventType eventType)
{
    if (mBridgeQueue != nullptr) {
        BridgeEventMsg msg{};
        msg.type = eventType;
        msg.schipHoogte = 0;
        msg.schipBreedte = 0;
        xQueueSend(mBridgeQueue, &msg, 0);
    }
}

void Actuator::taskEntry(void* pvParameters)
{
    auto* self = static_cast<Actuator*>(pvParameters);
    if (self) {
        self->taskLoop();
    }
    vTaskDelete(nullptr);
}

void Actuator::taskLoop()
{
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        if (mCommand == Command::OMHOOG) {
            std::printf("[Actuator] Command OMHOOG: OutOmhoog=1\n");
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 1);
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
            mBrug = true;
            mCommand = Command::NONE;
            sendEvent(BridgeEventType::BRUG_OPEN);
        } else if (mCommand == Command::OMLAAG) {
            std::printf("[Actuator] Command OMLAAG: OutOmlaag=1\n");
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 1);
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
            mBrug = false;
            mCommand = Command::NONE;
            sendEvent(BridgeEventType::BRUG_DICHT);
        } else if (mCommand == Command::STOP) {
            std::printf("[Actuator] Command STOP: outputs=0\n");
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
            mCommand = Command::NONE;
        }
    }
}

void Actuator::Omhoog()
{
    std::printf("[Actuator] Omhoog() aangeroepen.\n");
    mCommand = Command::OMHOOG;
}

void Actuator::Omlaag()
{
    std::printf("[Actuator] Omlaag() aangeroepen.\n");
    mCommand = Command::OMLAAG;
}

void Actuator::Stop()
{
    std::printf("[Actuator] Stop() aangeroepen.\n");
    mCommand = Command::STOP;
}

bool Actuator::GetActuatorPositie() const
{
    return mBrug;
}

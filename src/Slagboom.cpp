#include "Slagboom.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

Slagboom::Slagboom(int pinOutOmhoog, int pinOutOmlaag, int pinInOmhoog, int pinInOmlaag)
    : mSlagboom(false),
      mPinOutOmhoog(pinOutOmhoog),
      mPinOutOmlaag(pinOutOmlaag),
      mPinInOmhoog(pinInOmhoog),
      mPinInOmlaag(pinInOmlaag),
      mTask(nullptr),
      mBridgeQueue(nullptr),
      mCommand(Command::NONE)
{
    // Configure OUTPUT pins (commands: omhoog, omlaag)
    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << mPinOutOmhoog) | (1ULL << mPinOutOmlaag);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);

    // Configure INPUT pins (sensors: omhoog bereikt, omlaag bereikt)
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << mPinInOmhoog) | (1ULL << mPinInOmlaag);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    std::printf("[Slagboom] Geïnitialiseerd met:\n"
                "  OutOmhoog=%d, OutOmlaag=%d, InOmhoog=%d, InOmlaag=%d\n",
                mPinOutOmhoog, mPinOutOmlaag, mPinInOmhoog, mPinInOmlaag);
}

// Check: is the slagboom completely open (input sensor omhoog reads HIGH)?
bool Slagboom::isCompletelyOpen() const
{
    int level = gpio_get_level((gpio_num_t)mPinInOmhoog);
    return (level == 1);
}

// Check: is the slagboom completely closed (input sensor omlaag reads HIGH)?
bool Slagboom::isCompletelyClosed() const
{
    int level = gpio_get_level((gpio_num_t)mPinInOmlaag);
    return (level == 1);
}

void Slagboom::sendEvent(BridgeEventType eventType)
{
    if (mBridgeQueue != nullptr) {
        BridgeEventMsg msg{};
        msg.type = eventType;
        msg.schipHoogte = 0;
        msg.schipBreedte = 0;

        if (xQueueSend(mBridgeQueue, &msg, 0) == pdTRUE) {
            std::printf("[Slagboom] Event verstuurd: %d\n", (int)eventType);
        } else {
            std::printf("[Slagboom] WAARSCHUWING: kon event niet versturen!\n");
        }
    }
}

void Slagboom::taskEntry(void* pvParameters)
{
    auto* self = static_cast<Slagboom*>(pvParameters);
    if (self) {
        self->taskLoop();
    }
    vTaskDelete(nullptr);
}

void Slagboom::taskLoop()
{
    bool wasOpen = false;
    bool wasClosed = true;

    for (;;)
    {
        // Wacht kort zodat commands kunnen worden verwerkt
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms polling interval

        // Voer command uit als aanwezig
        if (mCommand == Command::OPEN) {
            std::printf("[Slagboom] Command OPEN: pinOutOmhoog = 1\n");
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 1);  // stuur omhoog command
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);  // zeker omlaag uit
            mCommand = Command::NONE;
        } else if (mCommand == Command::CLOSE) {
            std::printf("[Slagboom] Command CLOSE: pinOutOmlaag = 1\n");
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 1);  // stuur omlaag command
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);  // zeker omhoog uit
            mCommand = Command::NONE;
        }

        // Poll sensoren en stuur events als toestand verandert
        bool isOpen = isCompletelyOpen();
        bool isClosed = isCompletelyClosed();

        // Transities detecteren
        if (isOpen && !wasOpen) {
            std::printf("[Slagboom] Slagboom is nu OPEN (sensor InOmhoog = 1).\n");
            mSlagboom = true;
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);  // motor uit
            sendEvent(BridgeEventType::SLAGBOOM_OPEN);
            wasOpen = true;
            wasClosed = false;
        } else if (isClosed && !wasClosed) {
            std::printf("[Slagboom] Slagboom is nu DICHT (sensor InOmlaag = 1).\n");
            mSlagboom = false;
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);  // motor uit
            sendEvent(BridgeEventType::SLAGBOOM_DICHT);
            wasClosed = true;
            wasOpen = false;
        }
    }
}

// UML operations: command opslaan, taak zal dit uitvoeren

void Slagboom::OpenSlagboom()
{
    std::printf("[Slagboom] OpenSlagboom() aangeroepen.\n");
    mCommand = Command::OPEN;
}

void Slagboom::SluitSlagboom()
{
    std::printf("[Slagboom] SluitSlagboom() aangeroepen.\n");
    mCommand = Command::CLOSE;
}

bool Slagboom::GetSlagboomPositie() const
{
    return mSlagboom;
}

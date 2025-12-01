#include "VerkeersStoplicht.hpp"
#include "Commands.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
}

VerkeersStoplicht::VerkeersStoplicht(int pinR, int pinO, int pinG)
: mPinR(pinR),
  mPinO(pinO),
  mPinG(pinG),
  mRood(false),
  mOranje(false),
  mGroen(true),
  mState(State::GROEN),
  mCommandQueue(nullptr),
  mBridgeQueue(nullptr)
{
    gpio_config_t cfg{};
    cfg.intr_type    = GPIO_INTR_DISABLE;
    cfg.mode         = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask = (1ULL << mPinR) | (1ULL << mPinO) | (1ULL << mPinG);
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&cfg);

    // Start op GROEN
    setOutputs(false, false, true);

    printf("[VSL] Init op GROEN\n");

    mCommandQueue = xQueueCreate(5, sizeof(StoplichtCommandMsg));
}

void VerkeersStoplicht::setOutputs(bool rood, bool oranje, bool groen)
{
    gpio_set_level((gpio_num_t)mPinR, rood);
    gpio_set_level((gpio_num_t)mPinO, oranje);
    gpio_set_level((gpio_num_t)mPinG, groen);

    mRood   = rood;
    mOranje = oranje;
    mGroen  = groen;
}

void VerkeersStoplicht::sendEvent(BridgeEventType type)
{
    if (!mBridgeQueue) return;

    BridgeEventMsg msg{};
    msg.type = type;
    xQueueSend(mBridgeQueue, &msg, 0);
}

void VerkeersStoplicht::VerkeersStoplichtTask(void* pv)
{
    auto* self = static_cast<VerkeersStoplicht*>(pv);
    self->taskLoop();
    vTaskDelete(nullptr);
}

void VerkeersStoplicht::taskLoop()
{
    StoplichtCommandMsg cmd{};

    for (;;)
    {
        if (xQueueReceive(mCommandQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
            switch (cmd.cmd)
            {
                case StoplichtCommand::GROEN:
                    mState = State::GROEN;
                    setOutputs(false, false, true);
                    break;

                case StoplichtCommand::ROOD:
                    if (mState == State::GROEN)
                    {
                        // Eerst 2 sec oranje
                        mState = State::ORANJE;
                        setOutputs(false, true, false);
                        vTaskDelay(pdMS_TO_TICKS(2000));
                    }

                    // Dan pas rood
                    mState = State::ROOD;
                    setOutputs(true, false, false);

                    sendEvent(BridgeEventType::VERKEERSLICHT_ROOD);
                    break;

                default:
                    break;
            }
        }
    }
}

void VerkeersStoplicht::startTask(const char* name, UBaseType_t prio, uint32_t stack)
{
    xTaskCreate(
        VerkeersStoplichtTask,
        name,
        stack,
        this,
        prio,
        &mTask
    );
}

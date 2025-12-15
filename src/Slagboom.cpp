#include "Slagboom.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
}

//CONSTRUCTOR
Slagboom::Slagboom(int pinOutOmhoog, int pinOutOmlaag,
                   int pinInOmhoog, int pinInOmlaag)
    : mSlagboom(false),
      mPinOutOmhoog(pinOutOmhoog),
      mPinOutOmlaag(pinOutOmlaag),
      mPinInOmhoog(pinInOmhoog),
      mPinInOmlaag(pinInOmlaag),
      mTask(nullptr),
      mBridgeQueue(nullptr),
      mCommandQueue(nullptr),
      mTimeoutTimer(nullptr)
{
    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_DISABLE;

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << mPinOutOmhoog) | (1ULL << mPinOutOmlaag);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << mPinInOmhoog) | (1ULL << mPinInOmlaag);
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    mCommandQueue = xQueueCreate(10, sizeof(SlagboomCommandMsg));

    mTimeoutTimer = xTimerCreate(
        "SlagboomTimeout",
        pdMS_TO_TICKS(5000),   // 5 seconden
        pdFALSE,
        this,
        timeoutCallback
    );

    printf("[Slagboom] Init compleet\n");
}

//TIMEOUT
void Slagboom::startTimeout()
{
    if (mTimeoutTimer)
        xTimerStart(mTimeoutTimer, 0);
}

void Slagboom::stopTimeout()
{
    if (mTimeoutTimer)
        xTimerStop(mTimeoutTimer, 0);
}

void Slagboom::timeoutCallback(TimerHandle_t timer)
{
    auto* self = static_cast<Slagboom*>(pvTimerGetTimerID(timer));
    if (!self || !self->mBridgeQueue)
        return;

    printf("[Slagboom] TIMEOUT → NOODSTOP\n");

    BridgeEventMsg msg{};
    msg.type = BridgeEventType::NOODSTOP;
    xQueueSend(self->mBridgeQueue, &msg, 0);
}

//EVENTS
void Slagboom::sendEvent(BridgeEventType eventType)
{
    if (!mBridgeQueue) return;

    BridgeEventMsg msg{};
    msg.type = eventType;
    xQueueSend(mBridgeQueue, &msg, 0);
}

bool Slagboom::isCompletelyOpen() const
{
    return gpio_get_level((gpio_num_t)mPinInOmhoog);
}

bool Slagboom::isCompletelyClosed() const
{
    return gpio_get_level((gpio_num_t)mPinInOmlaag);
}

// TASK
void Slagboom::taskEntry(void* pvParameters)
{
    auto* self = static_cast<Slagboom*>(pvParameters);
    if (self)
        self->taskLoop();
    vTaskDelete(nullptr);
}

void Slagboom::taskLoop()
{
    bool wasOpen = false;
    bool wasClosed = true;

    SlagboomCommandMsg cmd{};
    cmd.cmd = SlagboomCommand::NONE;

    for (;;)
    {
        if (xQueueReceive(mCommandQueue, &cmd, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            switch (cmd.cmd)
            {
                case SlagboomCommand::OPEN:
                    printf("[Slagboom] OPEN\n");
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 1);
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
                    startTimeout();
                    break;

                case SlagboomCommand::CLOSE:
                    printf("[Slagboom] CLOSE\n");
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 1);
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
                    startTimeout();
                    break;

                case SlagboomCommand::STOP:
                    printf("[Slagboom] STOP\n");
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
                    stopTimeout();
                    break;

                default:
                    break;
            }

            cmd.cmd = SlagboomCommand::NONE;
        }

        bool isOpen = isCompletelyOpen();
        bool isClosed = isCompletelyClosed();

        if (isOpen && !wasOpen)
        {
            printf("[Slagboom] OPEN bereikt\n");
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
            stopTimeout();
            mSlagboom = true;
            sendEvent(BridgeEventType::SLAGBOOM_OPEN);
            wasOpen = true;
            wasClosed = false;
        }

        if (isClosed && !wasClosed)
        {
            printf("[Slagboom] DICHT bereikt\n");
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
            stopTimeout();
            mSlagboom = false;
            sendEvent(BridgeEventType::SLAGBOOM_DICHT);
            wasClosed = true;
            wasOpen = false;
        }
    }
}

// API
void Slagboom::OpenSlagboom()
{
    SlagboomCommandMsg msg{SlagboomCommand::OPEN};
    xQueueSend(mCommandQueue, &msg, 0);
}

void Slagboom::SluitSlagboom()
{
    SlagboomCommandMsg msg{SlagboomCommand::CLOSE};
    xQueueSend(mCommandQueue, &msg, 0);
}

bool Slagboom::GetSlagboomPositie() const
{
    return mSlagboom;
}

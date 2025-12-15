#include "Actuator.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "driver/gpio.h"
}

/* ---------------- CONSTRUCTOR ---------------- */
Actuator::Actuator(int pinOutOmhoog, int pinOutOmlaag,
                   int pinInOmhoog, int pinInOmlaag)
    : mState(State::IDLE),
      mBrug(false),
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
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

    uint64_t outputMask = (1ULL << mPinOutOmhoog) | (1ULL << mPinOutOmlaag);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = outputMask;
    gpio_config(&io_conf);

    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);

    uint64_t inputMask = (1ULL << mPinInOmhoog) | (1ULL << mPinInOmlaag);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = inputMask;
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    gpio_config(&io_conf);

    mCommandQueue = xQueueCreate(10, sizeof(ActuatorCommandMsg));

    mTimeoutTimer = xTimerCreate(
        "ActTimeout",
        pdMS_TO_TICKS(5000),   // 5 seconden
        pdFALSE,
        this,
        timeoutCallback
    );

    printf("[Actuator] Init compleet\n");
}

//TIMEOUT
void Actuator::startTimeout()
{
    if (mTimeoutTimer)
        xTimerStart(mTimeoutTimer, 0);
}

void Actuator::stopTimeout()
{
    if (mTimeoutTimer)
        xTimerStop(mTimeoutTimer, 0);
}

void Actuator::timeoutCallback(TimerHandle_t timer)
{
    auto* self = static_cast<Actuator*>(pvTimerGetTimerID(timer));
    if (!self || !self->mBridgeQueue)
        return;

    printf("[Actuator] TIMEOUT → NOODSTOP\n");

    BridgeEventMsg msg{};
    msg.type = BridgeEventType::NOODSTOP;
    xQueueSend(self->mBridgeQueue, &msg, 0);
}

//EVENT
void Actuator::sendEvent(BridgeEventType type)
{
    if (!mBridgeQueue) return;

    BridgeEventMsg msg{};
    msg.type = type;
    xQueueSend(mBridgeQueue, &msg, 0);
}

//TASK LOOP
void Actuator::taskLoop()
{
    ActuatorCommandMsg cmd{};

    for (;;)
    {
        if (xQueueReceive(mCommandQueue, &cmd, pdMS_TO_TICKS(20)) == pdTRUE)
        {
            switch (cmd.cmd)
            {
                case ActuatorCommand::OMHOOG:
                    printf("[Actuator] Start OMHOOG\n");
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 1);
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
                    mState = State::OMHOOG;
                    startTimeout();
                    break;

                case ActuatorCommand::OMLAAG:
                    printf("[Actuator] Start OMLAAG\n");
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 1);
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
                    mState = State::OMLAAG;
                    startTimeout();
                    break;

                case ActuatorCommand::STOP:
                    printf("[Actuator] STOP\n");
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
                    stopTimeout();
                    mState = State::IDLE;
                    break;

                default:
                    break;
            }
        }

        if (mState == State::OMHOOG &&
            gpio_get_level((gpio_num_t)mPinInOmhoog))
        {
            printf("[Actuator] Brug OPEN\n");
            gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
            stopTimeout();
            mState = State::IDLE;
            sendEvent(BridgeEventType::BRUG_OPEN);
        }

        if (mState == State::OMLAAG &&
            gpio_get_level((gpio_num_t)mPinInOmlaag))
        {
            printf("[Actuator] Brug DICHT\n");
            gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
            stopTimeout();
            mState = State::IDLE;
            sendEvent(BridgeEventType::BRUG_DICHT);
        }
        //FYSIEKE POSITIE ALTIJD BIJHOUDEN
        bool sensorOmhoog = gpio_get_level((gpio_num_t)mPinInOmhoog);
        bool sensorOmlaag = gpio_get_level((gpio_num_t)mPinInOmlaag);

        if (sensorOmhoog && !sensorOmlaag)
        {
            mBrug = true;
        }
        else if (sensorOmlaag && !sensorOmhoog)
        {
            mBrug = false;
        }

    }
}

//API
void Actuator::Omhoog()
{
    ActuatorCommandMsg msg{ActuatorCommand::OMHOOG};
    xQueueSend(mCommandQueue, &msg, 0);
}

void Actuator::Omlaag()
{
    ActuatorCommandMsg msg{ActuatorCommand::OMLAAG};
    xQueueSend(mCommandQueue, &msg, 0);
}

void Actuator::Stop()
{
    ActuatorCommandMsg msg{ActuatorCommand::STOP};
    xQueueSend(mCommandQueue, &msg, 0);
}

bool Actuator::GetActuatorPositie() const
{
    return mBrug;
}

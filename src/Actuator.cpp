#include "Actuator.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "driver/gpio.h"
}

Actuator::Actuator(int pinOutOmhoog, int pinOutOmlaag,
                   int pinInOmhoog, int pinInOmlaag)
    : mBrug(false),
      mPinOutOmhoog(pinOutOmhoog),
      mPinOutOmlaag(pinOutOmlaag),
      mPinInOmhoog(pinInOmhoog),
      mPinInOmlaag(pinInOmlaag),
      mTask(nullptr),
      mBridgeQueue(nullptr),
      mCommandQueue(nullptr)
{
    gpio_config_t io_conf{};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

    /* ---------------- OUTPUT PINS ---------------- */
    uint64_t outputMask = 0;
    if (mPinOutOmhoog >= 0) outputMask |= (1ULL << mPinOutOmhoog);
    if (mPinOutOmlaag >= 0) outputMask |= (1ULL << mPinOutOmlaag);

    if (outputMask) {
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = outputMask;
        gpio_config(&io_conf);

        if (mPinOutOmhoog >= 0) gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
        if (mPinOutOmlaag >= 0) gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
    }

    /* ---------------- INPUT PINS ---------------- */
    uint64_t inputMask = 0;
    if (mPinInOmhoog >= 0) inputMask |= (1ULL << mPinInOmhoog);
    if (mPinInOmlaag >= 0) inputMask |= (1ULL << mPinInOmlaag);

    if (inputMask) {
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = inputMask;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
        gpio_config(&io_conf);
    }

    printf("[Actuator] Init: OutUp=%d OutDown=%d InUp=%d InDown=%d\n",
           mPinOutOmhoog, mPinOutOmlaag, mPinInOmhoog, mPinInOmlaag);

    /* ---------------- QUEUE AANMAKEN ---------------- */
    mCommandQueue = xQueueCreate(10, sizeof(ActuatorCommandMsg));
    if (!mCommandQueue) {
        printf("[Actuator] ERROR: Command queue kon niet worden aangemaakt!\n");
    }
}

/* ---------------- EVENT TERUGSTUREN ---------------- */
void Actuator::sendEvent(BridgeEventType eventType)
{
    if (!mBridgeQueue) return;

    BridgeEventMsg msg{};
    msg.type = eventType;
    msg.schipHoogte = 0;
    msg.schipBreedte = 0;

    xQueueSend(mBridgeQueue, &msg, 0);
}

/* ---------------- DE ACTUELE ACTUATOR-LOOP ---------------- */
void Actuator::taskLoop()
{
    ActuatorCommandMsg cmd{};

    for (;;)
    {
        if (xQueueReceive(mCommandQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
            printf("[Actuator] Command: %d\n", (int)cmd.cmd);

            switch (cmd.cmd)
            {
                case ActuatorCommand::OMHOOG:
                {
                    printf("[Actuator] Brug gaat OMHOOG\n");

                    // Motor aan
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 1);
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);

                    // Wachten tot sensor omhoog HIGH wordt
                    while (gpio_get_level((gpio_num_t)mPinInOmhoog) == 0)
                    {
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }

                    // Motor uit
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);

                    mBrug = true;

                    printf("[Actuator] Brug is volledig OPEN\n");

                    sendEvent(BridgeEventType::BRUG_OPEN);
                    break;
                }

                case ActuatorCommand::OMLAAG:
                {
                    printf("[Actuator] Brug gaat OMLAAG\n");

                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 1);
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);

                    while (gpio_get_level((gpio_num_t)mPinInOmlaag) == 0)
                    {
                        vTaskDelay(pdMS_TO_TICKS(20));
                    }

                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);

                    mBrug = false;

                    printf("[Actuator] Brug is volledig DICHT\n");

                    sendEvent(BridgeEventType::BRUG_DICHT);
                    break;
                }

                case ActuatorCommand::STOP:
                    gpio_set_level((gpio_num_t)mPinOutOmhoog, 0);
                    gpio_set_level((gpio_num_t)mPinOutOmlaag, 0);
                    printf("[Actuator] STOP\n");
                    break;

                case ActuatorCommand::NONE:
                default:
                    break;
            }
        }
    }
}

/* ---------------- API FUNCTIES ---------------- */
void Actuator::Omhoog()
{
    ActuatorCommandMsg msg{ ActuatorCommand::OMHOOG };
    xQueueSend(mCommandQueue, &msg, 0);
}

void Actuator::Omlaag()
{
    ActuatorCommandMsg msg{ ActuatorCommand::OMLAAG };
    xQueueSend(mCommandQueue, &msg, 0);
}

void Actuator::Stop()
{
    ActuatorCommandMsg msg{ ActuatorCommand::STOP };
    xQueueSend(mCommandQueue, &msg, 0);
}

bool Actuator::GetActuatorPositie() const
{
    return mBrug;
}

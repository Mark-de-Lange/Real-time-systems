#include "Noodstop.hpp"
#include "Ophaalbrug.hpp"   // voor BridgeEventMsg en enum
#include <cstdio>

Noodstop::Noodstop(int pin)
    : mPin(static_cast<gpio_num_t>(pin))
{
    gpio_config_t cfg{};
    cfg.pin_bit_mask = (1ULL << mPin);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_NEGEDGE;

    gpio_config(&cfg);

    // ISR registreren
    gpio_isr_handler_add(mPin, &Noodstop::isrHandler, this);
}


void Noodstop::setBridgeQueue(QueueHandle_t queue)
{
    mBridgeQueue = queue;
}

// ISR
void IRAM_ATTR Noodstop::isrHandler(void* arg)
{
    auto* self = static_cast<Noodstop*>(arg);
    if (!self || !self->mBridgeQueue)
        return;

    BridgeEventMsg msg{};
    msg.type = BridgeEventType::NOODSTOP;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(self->mBridgeQueue, &msg, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
        portYIELD_FROM_ISR();
}

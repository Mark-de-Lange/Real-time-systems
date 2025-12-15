#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

// Forward declaration
struct BridgeEventMsg;

class Noodstop {
public:
    explicit Noodstop(int pin);

    void setBridgeQueue(QueueHandle_t queue);

private:
    static void IRAM_ATTR isrHandler(void* arg);

    gpio_num_t   mPin;
    QueueHandle_t mBridgeQueue{nullptr};
};

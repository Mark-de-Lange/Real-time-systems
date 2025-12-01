#ifndef ACTUATOR_HPP
#define ACTUATOR_HPP

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
    #include "freertos/task.h"
    #include "driver/gpio.h"
}

#include "pinConfig.hpp"
#include "BridgeEvents.hpp"
#include "Commands.hpp"

class Actuator {
private:
    bool mBrug;
    int  mPinOutOmhoog;
    int  mPinOutOmlaag;
    int  mPinInOmhoog;
    int  mPinInOmlaag;

    TaskHandle_t mTask;
    QueueHandle_t mBridgeQueue;
    QueueHandle_t mCommandQueue;

    void taskLoop();         // NIET static!
    void sendEvent(BridgeEventType type);

public:
    Actuator(int pinOutOmhoog,
             int pinOutOmlaag,
             int pinInOmhoog,
             int pinInOmlaag);

    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }
    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    // 🔥 Dit is de ENIGE FreeRTOS task entry voor de actuator
    static void ActuatorTask(void* pvParameters) {
        auto* self = static_cast<Actuator*>(pvParameters);
        if (self)
            self->taskLoop();
        vTaskDelete(nullptr);
    }

    void Omhoog();
    void Omlaag();
    void Stop();
    bool GetActuatorPositie() const;
};

#endif

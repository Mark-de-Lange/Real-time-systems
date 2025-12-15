#ifndef ACTUATOR_HPP
#define ACTUATOR_HPP

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "freertos/timers.h"
    #include "driver/gpio.h"
}

#include "BridgeEvents.hpp"
#include "Commands.hpp"

class Actuator {
public:
    enum class State {
        IDLE,
        OMHOOG,
        OMLAAG
    };

    Actuator(int pinOutOmhoog,
             int pinOutOmlaag,
             int pinInOmhoog,
             int pinInOmlaag);

    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }
    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    static void ActuatorTask(void* pvParameters)
    {
        auto* self = static_cast<Actuator*>(pvParameters);
        if (self)
            self->taskLoop();
        vTaskDelete(nullptr);
    }

    void Omhoog();
    void Omlaag();
    void Stop();
    bool GetActuatorPositie() const;

private:
    // toestand
    State mState;
    bool  mBrug;

    // hardware
    int mPinOutOmhoog;
    int mPinOutOmlaag;
    int mPinInOmhoog;
    int mPinInOmlaag;

    // RTOS
    TaskHandle_t  mTask;
    QueueHandle_t mBridgeQueue;
    QueueHandle_t mCommandQueue;

    // timeout
    TimerHandle_t mTimeoutTimer;
    static void timeoutCallback(TimerHandle_t timer);
    void startTimeout();
    void stopTimeout();

    void taskLoop();
    void sendEvent(BridgeEventType type);
};

#endif

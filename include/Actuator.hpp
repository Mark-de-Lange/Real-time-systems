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

// UML: Actuator
// - mBrug : Boolean (true = omhoog/open)
// - mPinOutOmhoog : Integer
// - mPinOutOmlaag : Integer
// - mPinInOmhoog  : Integer
// - mPinInOmlaag  : Integer
// + Omhoog(), Omlaag(), GetActuatorPositie()

class Actuator {
private:
    bool mBrug;
    int  mPinOutOmhoog;
    int  mPinOutOmlaag;
    int  mPinInOmhoog;
    int  mPinInOmlaag;

    TaskHandle_t mTask;
    QueueHandle_t mBridgeQueue;

    enum class Command {
        NONE,
        OMHOOG,
        OMLAAG,
        STOP
    };
    Command mCommand;

    static void taskEntry(void* pvParameters);
    void taskLoop();

    void sendEvent(BridgeEventType eventType);

public:
    explicit Actuator(
        int pinOutOmhoog = (int)PIN_BRUG_ACTUATOR,
        int pinOutOmlaag = (int)PIN_BRUG_ACTUATOR,
        int pinInOmhoog  = -1,
        int pinInOmlaag  = -1
    );

    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

    static void ActuatorTask(void* pvParameters) { taskEntry(pvParameters); }

    void Omhoog();
    void Omlaag();
    void Stop();
    bool GetActuatorPositie() const;
};

#endif // ACTUATOR_HPP
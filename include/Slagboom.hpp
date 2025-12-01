#ifndef SLAGBOOM_HPP
#define SLAGBOOM_HPP

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
    #include "freertos/task.h"
    #include "driver/gpio.h"
}

#include "pinConfig.hpp"
#include "BridgeEvents.hpp"

// UML: Slagboom
// - mSlagboom       : Boolean (true = omhoog/open)
// - mPinOutOmhoog   : Integer (output: slagboom omhoog)
// - mPinOutOmlaag   : Integer (output: slagboom omlaag)
// - mPinInOmhoog    : Integer (input sensor: omhoog bereikt)
// - mPinInOmlaag    : Integer (input sensor: omlaag bereikt)
// + Slagboom()
// + OpenSlagboom()
// + SluitSlagboom()
// + GetSlagboomPositie() : Boolean

class Slagboom {
private:
    // UML attributes
    bool mSlagboom;           // true = omhoog/open, false = omlaag/dicht
    int  mPinOutOmhoog;       // output: command omhoog
    int  mPinOutOmlaag;       // output: command omlaag
    int  mPinInOmhoog;        // input sensor: bereikt omhoog
    int  mPinInOmlaag;        // input sensor: bereikt omlaag

    TaskHandle_t mTask;          // eigen FreeRTOS taak
    QueueHandle_t mBridgeQueue;  // referentie naar Ophaalbrug event queue

    // Command enum (simpel)
    enum class Command {
        NONE,
        OPEN,
        CLOSE
    };
    Command mCommand;

    // FreeRTOS task entry + loop
    static void taskEntry(void* pvParameters);
    void taskLoop();

    // interne helpers
    bool isCompletelyOpen() const;
    bool isCompletelyClosed() const;
    void sendEvent(BridgeEventType eventType);

public:
    // «create» Slagboom()
    Slagboom(int pinOutOmhoog, int pinOutOmlaag, int pinInOmhoog, int pinInOmlaag);

    // Wordt aangeroepen vanuit main ná xTaskCreate
    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

    // Functie die je in main aan xTaskCreate geeft
    static void SlagboomTask(void* pvParameters) {
        taskEntry(pvParameters);
    }

    // UML operations
    void OpenSlagboom();
    void SluitSlagboom();
    bool GetSlagboomPositie() const;
};

#endif // SLAGBOOM_HPP

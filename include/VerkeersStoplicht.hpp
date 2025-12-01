#ifndef VERKEERSSTOPLICHT_HPP
#define VERKEERSSTOPLICHT_HPP

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
    #include "driver/gpio.h"
}

#include "BridgeEvents.hpp"
#include "Commands.hpp"

class VerkeersStoplicht
{
public:
    // De drie mogelijke toestanden van het stoplicht
    enum class State {
        GROEN,
        ORANJE,
        ROOD
    };

private:
    // GPIO pinnen
    int mPinR;
    int mPinO;
    int mPinG;

    // Huidige kleurstatus
    bool mRood;
    bool mOranje;
    bool mGroen;

    // Interne state voor logica
    State mState;

    // FreeRTOS
    TaskHandle_t mTask = nullptr;
    QueueHandle_t mCommandQueue = nullptr;     // opdrachten van Ophaalbrug
    QueueHandle_t mBridgeQueue = nullptr;      // events terug naar Ophaalbrug

    // Hulpfuncties
    void setOutputs(bool rood, bool oranje, bool groen);
    void sendEvent(BridgeEventType type);
    void taskLoop();

public:
    VerkeersStoplicht(int pinR, int pinO, int pinG);

    // Queue setter voor centrale Ophaalbrug event queue
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

    // Toegang tot de command queue
    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    // Start de FreeRTOS task
    void startTask(const char* name = "VerkeersStoplichtTask",
                   UBaseType_t prio = 5,
                   uint32_t stackSize = 4096);

    // FreeRTOS thread wrapper
    static void VerkeersStoplichtTask(void* pvParameters);
};

#endif // VERKEERSSTOPLICHT_HPP

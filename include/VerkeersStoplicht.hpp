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
    enum class State {
        GROEN,
        ORANJE,
        ROOD
    };

private:
    int mPinR;
    int mPinO;
    int mPinG;

    bool mRood;
    bool mOranje;
    bool mGroen;

    State mState;

    TaskHandle_t mTask = nullptr;
    QueueHandle_t mCommandQueue = nullptr;     // opdrachten van Ophaalbrug
    QueueHandle_t mBridgeQueue = nullptr;      // events terug naar Ophaalbrug

    void setOutputs(bool rood, bool oranje, bool groen);
    void sendEvent(BridgeEventType type);
    void taskLoop();

public:
    VerkeersStoplicht(int pinR, int pinO, int pinG);

    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    void startTask(const char* name = "VerkeersStoplichtTask",
                   UBaseType_t prio = 5,
                   uint32_t stackSize = 4096);

    static void VerkeersStoplichtTask(void* pvParameters);
};

#endif // VERKEERSSTOPLICHT_HPP

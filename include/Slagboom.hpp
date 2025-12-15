#ifndef SLAGBOOM_HPP
#define SLAGBOOM_HPP

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

class Slagboom {
private:
    bool mSlagboom;        // true = open
    int  mPinOutOmhoog;
    int  mPinOutOmlaag;
    int  mPinInOmhoog;
    int  mPinInOmlaag;

    TaskHandle_t  mTask;
    QueueHandle_t mBridgeQueue;
    QueueHandle_t mCommandQueue;

    // timeout
    TimerHandle_t mTimeoutTimer;
    static void timeoutCallback(TimerHandle_t timer);
    void startTimeout();
    void stopTimeout();

    static void taskEntry(void* pvParameters);
    void taskLoop();

    bool isCompletelyOpen() const;
    bool isCompletelyClosed() const;
    void sendEvent(BridgeEventType eventType);

public:
    Slagboom(int pinOutOmhoog, int pinOutOmlaag,
             int pinInOmhoog, int pinInOmlaag);

    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }
    void setBridgeQueue(QueueHandle_t q) { mBridgeQueue = q; }

    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    static void SlagboomTask(void* pvParameters)
    {
        taskEntry(pvParameters);
    }

    void OpenSlagboom();
    void SluitSlagboom();
    bool GetSlagboomPositie() const;
};

#endif

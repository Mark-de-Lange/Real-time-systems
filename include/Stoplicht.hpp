#ifndef STOPLICHT_HPP
#define STOPLICHT_HPP

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
}

#include "Commands.hpp"

class Stoplicht {
protected:
    bool mDoorlaat;
    bool mTegenhouden;
    TaskHandle_t mTask;
    QueueHandle_t mCommandQueue;

public:
    Stoplicht();
    virtual ~Stoplicht() = default;

    void SetDoorlaat(bool state);
    void SetTegenhouden(bool state);

    TaskHandle_t getTaskHandle() const { return mTask; }
    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }

    void setBridgeQueue(void* q) {}

protected:
    virtual void FaseHandler() = 0;
};

#endif

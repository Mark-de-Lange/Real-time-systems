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
    bool mDoorlaat;           // = groen
    bool mTegenhouden;        // = rood
    TaskHandle_t mTask;       // wordt gezet door startTask()
    QueueHandle_t mCommandQueue;  // command queue van Ophaalbrug

public:
    Stoplicht();
    virtual ~Stoplicht() = default;

    // Deze 2 setters sturen een notify bij elke toestand-verandering
    void SetDoorlaat(bool state);
    void SetTegenhouden(bool state);

    // Utilities
    TaskHandle_t getTaskHandle() const { return mTask; }
    void setTaskHandle(TaskHandle_t handle) { mTask = handle; }

    void setBridgeQueue(void* q) {}  // (optioneel voor latere uitbreidingen)

protected:
    // Wordt implementiert door subklassen (zoals VerkeersStoplicht)
    virtual void FaseHandler() = 0;
};

#endif

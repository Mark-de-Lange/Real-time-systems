#ifndef STOPLICHT_HPP
#define STOPLICHT_HPP

#include <cstdint>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
}

// Basisklasse volgens UML:
// - mDoorlaat     : Boolean
// - mTegenhouden  : Boolean
// + SetDoorlaat(bool)
// + SetTegenhouden(bool)
// + eigen FreeRTOS-task

class Stoplicht {
protected:
    bool          mDoorlaat;      // true = groen / doorlaten
    bool          mTegenhouden;   // true = rood / tegenhouden
    TaskHandle_t  mTask;          // eigen FreeRTOS-task

    // FreeRTOS task entry
    static void taskEntry(void* pv);
    // Eigen loop: wachten op notify en dan FaseHandler() aanroepen
    void taskLoop();

    // Moet door afgeleide klassen worden geïmplementeerd:
    virtual void FaseHandler() = 0;

public:
    Stoplicht();
    virtual ~Stoplicht() = default;

    // Task starten
    void startTask(const char* name, UBaseType_t prio, uint32_t stackWords);

    // UML-methodes
    void SetDoorlaat(bool v);
    void SetTegenhouden(bool v);
};

#endif // STOPLICHT_HPP

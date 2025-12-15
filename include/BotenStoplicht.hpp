#ifndef BOTEN_STOPLICHT_HPP
#define BOTEN_STOPLICHT_HPP

#include "Stoplicht.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
    #include "driver/gpio.h"
}

class BotenStoplicht : public Stoplicht {
private:
    bool mRood;
    bool mGroen;

    int  mPinR;
    int  mPinG;

    bool mPrevDoorlaat;
    bool mPrevTegenhouden;

    QueueHandle_t mCommandQueue;

protected:
    void FaseHandler() override;

public:
    BotenStoplicht(int pinR, int pinG);

    int GetFase() const;
    void SetKleur(int kleur);

    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    // Task
    static void BotenStoplichtTask(void* pv);
    void startTask(const char* name, UBaseType_t prio, uint32_t stack);
};

#endif // BOTEN_STOPLICHT_HPP

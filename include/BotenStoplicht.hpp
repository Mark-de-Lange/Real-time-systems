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

    // Voor overgangsdetectie (niet essentieel zoals bij verkeer)
    bool mPrevDoorlaat;
    bool mPrevTegenhouden;

    // Queue voor opdrachten van de Ophaalbrug. Net als bij het
    // verkeersstoplicht wordt deze in de constructor aangemaakt en
    // bevat ze StoplichtCommandMsg berichten.
    QueueHandle_t mCommandQueue;

protected:
    void FaseHandler() override;

public:
    BotenStoplicht(int pinR, int pinG);

    int GetFase() const;
    void SetKleur(int kleur);

    // Geeft de command-queue terug zodat de Ophaalbrug opdrachten kan
    // versturen. De queue bevat StoplichtCommandMsg structuren.
    QueueHandle_t getCommandQueue() const { return mCommandQueue; }

    // Task
    static void BotenStoplichtTask(void* pv);
    void startTask(const char* name, UBaseType_t prio, uint32_t stack);
};

#endif // BOTEN_STOPLICHT_HPP

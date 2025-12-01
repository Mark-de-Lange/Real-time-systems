#ifndef VERKEERS_STOPLICHT_HPP
#define VERKEERS_STOPLICHT_HPP

#include "Stoplicht.hpp"

extern "C" {
    #include "driver/gpio.h"
}

// UML VerkeersStoplicht:
// - Rood   : Boolean
// - Oranje : Boolean
// - Groen  : Boolean
// - mPinR  : Integer
// - mPinO  : Integer
// - mPinG  : Integer
// + VerkeersStoplicht()
// + FaseHandler()
// + GetFase() : Integer
// + SetKleur(Enum)

class VerkeersStoplicht : public Stoplicht {
private:
    bool mRood;
    bool mOranje;
    bool mGroen;

    int  mPinR;
    int  mPinO;
    int  mPinG;

protected:
    void FaseHandler() override;

public:
    // «create» VerkeersStoplicht()
    VerkeersStoplicht(int pinR, int pinO, int pinG);

    // 0 = rood, 1 = oranje, 2 = groen, -1 = alles uit
    int GetFase() const;

    // eenvoudige kleur-keuze:
    // 0 = rood, 1 = oranje, 2 = groen
    void SetKleur(int kleur);
};

#endif // VERKEERS_STOPLICHT_HPP

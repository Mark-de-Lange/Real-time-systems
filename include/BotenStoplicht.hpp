#ifndef BOTEN_STOPLICHT_HPP
#define BOTEN_STOPLICHT_HPP

#include "Stoplicht.hpp"

extern "C" {
    #include "driver/gpio.h"
}

// UML BotenStoplicht:
// - Rood : Boolean
// - Groen: Boolean
// - mPinR: Integer
// - mPinG: Integer
// + BotenStoplicht()
// + FaseHandler()
// + GetFase() : Integer
// + SetKleur(Enum)

class BotenStoplicht : public Stoplicht {
private:
    bool mRood;
    bool mGroen;

    int  mPinR;
    int  mPinG;

protected:
    void FaseHandler() override;

public:
    // «create» BotenStoplicht()
    BotenStoplicht(int pinR, int pinG);

    // 0 = rood, 1 = groen, -1 = uit
    int GetFase() const;

    // 0 = rood, 1 = groen
    void SetKleur(int kleur);
};

#endif // BOTEN_STOPLICHT_HPP

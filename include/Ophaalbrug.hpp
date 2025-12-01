#ifndef OPHAALBRUG_HPP
#define OPHAALBRUG_HPP

#include <cstdint>
#include "BridgeEvents.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
}

#include "Slagboom.hpp"
#include "Actuator.hpp"
#include "VerkeersStoplicht.hpp"
#include "BotenStoplicht.hpp"
#include "Commands.hpp"

class Ophaalbrug
{
private:
    // Objecten uit UML
    Slagboom&          mSlagboom;
    VerkeersStoplicht& mVerkeersStoplicht;
    BotenStoplicht&    mBotenStoplicht;
    Actuator&          mActuator;

    int mMaximaleHoogte;
    int mMaximaleBreedte;

    QueueHandle_t mEventQueue;

    // COMPLETE state-machine
    enum class State {
        IDLE,
        WACHT_OP_VERKEERSLICHT_ROOD,
        WACHT_OP_SLAGBOOM_DICHT,
        WACHT_OP_BRUG_OPEN,
        OPEN,
        WACHT_OP_BRUG_DICHT,
        WACHT_OP_SLAGBOOM_OPEN
    };
    State mState;

    int mLaatsteSchipHoogte;
    int mLaatsteSchipBreedte;

    static void taskEntry(void* pvParameters);
    void taskLoop();

    // De functies die jouw .cpp wél bevat:
    void startOpenProces();
    void startSluitProces();
    void onEvent(const BridgeEventMsg& msg);

public:
    Ophaalbrug(Slagboom& slagboom,
               VerkeersStoplicht& vStop,
               BotenStoplicht& bStop,
               Actuator& actuator,
               int maximaleHoogte,
               int maximaleBreedte);

    static void OphaalbrugTask(void* pvParameters) {
        taskEntry(pvParameters);
    }

    QueueHandle_t getEventQueue() const { return mEventQueue; }

    // UML operations
    void openBrug();
    void sluitBrug();
    void noodstop();
};

#endif // OPHAALBRUG_HPP

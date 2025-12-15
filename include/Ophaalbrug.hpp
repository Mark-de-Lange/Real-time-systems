#ifndef OPHAALBRUG_HPP
#define OPHAALBRUG_HPP

#include <deque>
#include <cstdint>
#include "BridgeEvents.hpp"
#include "Slagboom.hpp"
#include "VerkeersStoplicht.hpp"
#include "BotenStoplicht.hpp"
#include "Actuator.hpp"

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
}

// Struct voor schip-wachtrij
struct SchipInfo {
    int hoogte;
    int breedte;
    bool brugMoetOpen;
};

class Ophaalbrug
{
public:
    enum class State {
        IDLE,
        WACHT_OP_VERKEERSLICHT_ROOD,
        WACHT_OP_SLAGBOOM_DICHT,
        WACHT_OP_BRUG_OPEN,
        OPEN,
        WACHT_OP_BRUG_DICHT,
        WACHT_OP_SLAGBOOM_OPEN
    };

    Ophaalbrug(Slagboom& slagboom,
               VerkeersStoplicht& verkeersStoplicht,
               BotenStoplicht& botenStoplicht,
               Actuator& actuator,
               int maximaleHoogte,
               int maximaleBreedte);

    QueueHandle_t getEventQueue() const { return mEventQueue; }

    // FreeRTOS task wrapper
    static void OphaalbrugTask(void* pv) {
        static_cast<Ophaalbrug*>(pv)->taskLoop();
    }

private:
    // Hardware componenten
    Slagboom&        mSlagboom;
    VerkeersStoplicht& mVerkeersStoplicht;
    BotenStoplicht&  mBotenStoplicht;
    Actuator&        mActuator;

    // Bruglimieten
    int mMaximaleHoogte;
    int mMaximaleBreedte;

    // Brugstatus
    State mState;

    // Wachtrij met schepen
    std::deque<SchipInfo> mSchipQueue;

    // Event queue
    QueueHandle_t mEventQueue;

    // Hoofdloop
    void taskLoop();
    void onEvent(const BridgeEventMsg& msg);

    // Brugproces
    void startOpenProces();
    void startSluitProces();

    // Centrale beslisfunctie
    void updateBrugEnStoplichten();

    // Noodstop
    void noodstop();
};

#endif

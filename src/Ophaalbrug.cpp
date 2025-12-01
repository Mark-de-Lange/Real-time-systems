#include "Ophaalbrug.hpp"
#include <cstdio>

Ophaalbrug::Ophaalbrug(Slagboom& slagboom,
                       VerkeersStoplicht& vStop,
                       BotenStoplicht& bStop,
                       Actuator& actuator,
                       int maximaleHoogte,
                       int maximaleBreedte)
: mSlagboom(slagboom),
  mVerkeersStoplicht(vStop),
  mBotenStoplicht(bStop),
  mActuator(actuator),
  mMaximaleHoogte(maximaleHoogte),
  mMaximaleBreedte(maximaleBreedte),
  mEventQueue(nullptr),
  mState(State::IDLE),
  mLaatsteSchipHoogte(0),
  mLaatsteSchipBreedte(0)
{
    mEventQueue = xQueueCreate(10, sizeof(BridgeEventMsg));
    if (mEventQueue == nullptr) {
        std::printf("[Ophaalbrug] ERROR: kon event queue niet maken!\n");
    }
}

void Ophaalbrug::taskEntry(void* pvParameters)
{
    auto* self = static_cast<Ophaalbrug*>(pvParameters);
    if (self) {
        self->taskLoop();
    }
    vTaskDelete(nullptr);
}

void Ophaalbrug::taskLoop()
{
    BridgeEventMsg msg{};

    for (;;)
    {
        if (xQueueReceive(mEventQueue, &msg, portMAX_DELAY) == pdTRUE) {
            onEvent(msg);
        }
    }
}

// --------- UML-operations ----------

void Ophaalbrug::openBrug()
{
    // Check: schip moet binnen maximale maten vallen
    if (mLaatsteSchipHoogte > mMaximaleHoogte ||
        mLaatsteSchipBreedte > mMaximaleBreedte)
    {
        std::printf("[Ophaalbrug] Schip te groot: "
                    "hoogte=%d (max=%d), breedte=%d (max=%d). Brug blijft dicht.\n",
                    mLaatsteSchipHoogte, mMaximaleHoogte,
                    mLaatsteSchipBreedte, mMaximaleBreedte);
        return;
    }

    std::printf("[Ophaalbrug] openBrug() gestart.\n");

    // 1) Verkeer tegenhouden (rood verkeerslicht)
    mVerkeersStoplicht.SetTegenhouden(true);

    // 2) Slagboom sluiten
    mSlagboom.SluitSlagboom();

    mState = State::WACHT_OP_SLAGBOOM_DICHT;
}

void Ophaalbrug::sluitBrug()
{
    std::printf("[Ophaalbrug] sluitBrug() gestart.\n");

    // 1) Boten tegenhouden (rood botenlicht)
    mBotenStoplicht.SetTegenhouden(true);

    // 2) Brug omlaag
    mActuator.Omlaag();

    mState = State::WACHT_OP_BRUG_DICHT;
}

void Ophaalbrug::noodstop()
{
    std::printf("[Ophaalbrug] NOODSTOP!\n");

    // Alles in veilige toestand
    mActuator.Stop();                     // vergeet niet in Actuator te maken
    mSlagboom.SluitSlagboom();
    mVerkeersStoplicht.SetTegenhouden(true);
    mBotenStoplicht.SetTegenhouden(true);

    mState = State::IDLE;
}

// --------- Event-afhandeling ----------

void Ophaalbrug::onEvent(const BridgeEventMsg& msg)
{
    switch (msg.type)
    {
        case BridgeEventType::SCHIP_GEDETECTEERD:
            mLaatsteSchipHoogte  = msg.schipHoogte;
            mLaatsteSchipBreedte = msg.schipBreedte;

            if (mState == State::IDLE) {
                openBrug();
            }
            break;

        case BridgeEventType::SLAGBOOM_DICHT:
            if (mState == State::WACHT_OP_SLAGBOOM_DICHT) {
                std::printf("[Ophaalbrug] Slagboom is dicht, brug gaat omhoog.\n");
                mActuator.Omhoog();
                mState = State::WACHT_OP_BRUG_OPEN;
            }
            break;

        case BridgeEventType::BRUG_OPEN:
            if (mState == State::WACHT_OP_BRUG_OPEN) {
                std::printf("[Ophaalbrug] Brug is open, boten mogen door (groen).\n");
                mBotenStoplicht.SetDoorlaat(true); // groen voor boten
                mState = State::OPEN;
            }
            break;

        case BridgeEventType::GEEN_WACHTENDE_SCHEPEN:
            if (mState == State::OPEN) {
                sluitBrug();
            }
            break;

        case BridgeEventType::BRUG_DICHT:
            if (mState == State::WACHT_OP_BRUG_DICHT) {
                std::printf("[Ophaalbrug] Brug is dicht, slagboom open.\n");
                mSlagboom.OpenSlagboom();
                mState = State::WACHT_OP_SLAGBOOM_OPEN;
            }
            break;

        case BridgeEventType::SLAGBOOM_OPEN:
            if (mState == State::WACHT_OP_SLAGBOOM_OPEN) {
                std::printf("[Ophaalbrug] Slagboom is open, verkeer mag door (groen).\n");
                mVerkeersStoplicht.SetDoorlaat(true); // groen voor verkeer
                mState = State::IDLE;
            }
            break;

        case BridgeEventType::NOODSTOP:
            noodstop();
            break;
    }
}

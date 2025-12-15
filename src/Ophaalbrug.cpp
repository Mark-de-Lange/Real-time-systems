#include "Ophaalbrug.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
}

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

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
  mState(State::IDLE)
{
    mEventQueue = xQueueCreate(20, sizeof(BridgeEventMsg));

    if (!mEventQueue)
        printf("[Ophaalbrug] ERROR: kon eventqueue niet maken!\n");
}

// -----------------------------------------------------------------------------
// Task Loop
// -----------------------------------------------------------------------------

void Ophaalbrug::taskLoop()
{
    BridgeEventMsg msg{};

    for (;;)
    {
        if (xQueueReceive(mEventQueue, &msg, portMAX_DELAY) == pdTRUE)
        {
            onEvent(msg);
        }
    }
}

// -----------------------------------------------------------------------------
// Start Brug Open Proces
// -----------------------------------------------------------------------------

void Ophaalbrug::startOpenProces()
{
    printf("[Ophaalbrug] Start OPEN proces...\n");

    // Auto-stoplichten naar rood (auto)
    StoplichtCommandMsg v{};
    v.cmd = StoplichtCommand::ROOD;
    xQueueSend(mVerkeersStoplicht.getCommandQueue(), &v, 0);

    mState = State::WACHT_OP_VERKEERSLICHT_ROOD;
}

// -----------------------------------------------------------------------------
// Start Brug Sluit Proces
// -----------------------------------------------------------------------------

void Ophaalbrug::startSluitProces()
{
    printf("[Ophaalbrug] Start SLUIT proces...\n");

    // 1. Boten op rood
    StoplichtCommandMsg b{};
    b.cmd = StoplichtCommand::ROOD;
    xQueueSend(mBotenStoplicht.getCommandQueue(), &b, 0);

    // 2. Brug omlaag
    ActuatorCommandMsg a{};
    a.cmd = ActuatorCommand::OMLAAG;
    xQueueSend(mActuator.getCommandQueue(), &a, 0);

    mState = State::WACHT_OP_BRUG_DICHT;
}

// -----------------------------------------------------------------------------
// NOODSTOP
// -----------------------------------------------------------------------------

void Ophaalbrug::noodstop()
{
    printf("[Ophaalbrug] !!! NOODSTOP !!!\n");

    // Brugmotor stoppen
    ActuatorCommandMsg a{};
    a.cmd = ActuatorCommand::STOP;
    xQueueSend(mActuator.getCommandQueue(), &a, 0);

    // Slagboom sluiten
    SlagboomCommandMsg s{};
    s.cmd = SlagboomCommand::CLOSE;
    xQueueSend(mSlagboom.getCommandQueue(), &s, 0);

    // Alle stoplichten op rood
    StoplichtCommandMsg l{};
    l.cmd = StoplichtCommand::ROOD;
    xQueueSend(mVerkeersStoplicht.getCommandQueue(), &l, 0);
    xQueueSend(mBotenStoplicht.getCommandQueue(),    &l, 0);

    mState = State::IDLE;
}

// -----------------------------------------------------------------------------
// Centrale Brug & Stoplicht Logic
// -----------------------------------------------------------------------------

void Ophaalbrug::updateBrugEnStoplichten()
{
    if (mSchipQueue.empty())
    {
        printf("[Ophaalbrug] Wachtrij leeg → brug mag dicht.\n");

        // Als brug open is, sluiten
        if (mState == State::OPEN)
            startSluitProces();

        // Boten rood
        StoplichtCommandMsg sm{};
        sm.cmd = StoplichtCommand::ROOD;
        xQueueSend(mBotenStoplicht.getCommandQueue(), &sm, 0);

        return;
    }

    SchipInfo& schip = mSchipQueue.front();

    printf("[Ophaalbrug] Voorste schip → hoogte=%d open=%d\n",
           schip.hoogte, schip.brugMoetOpen);

    // Schip past onder de brug
    if (!schip.brugMoetOpen)
    {
        StoplichtCommandMsg sm{};
        sm.cmd = StoplichtCommand::GROEN;
        xQueueSend(mBotenStoplicht.getCommandQueue(), &sm, 0);
        return;
    }

    // Schip moet brug open
    StoplichtCommandMsg sm{};
    sm.cmd = StoplichtCommand::ROOD;
    xQueueSend(mBotenStoplicht.getCommandQueue(), &sm, 0);

    if (mState == State::IDLE)
        startOpenProces();
}

// -----------------------------------------------------------------------------
// Event Handler
// -----------------------------------------------------------------------------

void Ophaalbrug::onEvent(const BridgeEventMsg& msg)
{
    switch (msg.type)
    {
        // ---------------------------------------------------------------------
        case BridgeEventType::SCHIP_GEDETECTEERD: {
            SchipInfo nieuw{
                msg.schipHoogte,
                msg.schipBreedte,
                msg.schipHoogte > mMaximaleHoogte
            };

            printf("[Ophaalbrug] Schip gedetecteerd: H=%d B=%d open=%d\n",
                   nieuw.hoogte, nieuw.breedte, nieuw.brugMoetOpen);

            mSchipQueue.push_back(nieuw);
            updateBrugEnStoplichten();
        }
        break;

        // ---------------------------------------------------------------------
        case BridgeEventType::SCHIP_AFGEMELD:
            if (!mSchipQueue.empty())
            {
                printf("[Ophaalbrug] Schip afgemeld.\n");
                mSchipQueue.pop_front();
            }
            else
                printf("[Ophaalbrug] Afmelden maar geen schepen.\n");

            updateBrugEnStoplichten();
            break;

        // ---------------------------------------------------------------------
        case BridgeEventType::VERKEERSLICHT_ROOD:
            if (mState == State::WACHT_OP_VERKEERSLICHT_ROOD)
            {
                printf("[Ophaalbrug] Verkeerslicht rood → slagboom omlaag.\n");

                SlagboomCommandMsg s{};
                s.cmd = SlagboomCommand::CLOSE;
                xQueueSend(mSlagboom.getCommandQueue(), &s, 0);

                mState = State::WACHT_OP_SLAGBOOM_DICHT;
            }
            break;

        // ---------------------------------------------------------------------
        case BridgeEventType::SLAGBOOM_DICHT:
            if (mState == State::WACHT_OP_SLAGBOOM_DICHT)
            {
                printf("[Ophaalbrug] Slagboom dicht → brug omhoog.\n");

                ActuatorCommandMsg a{};
                a.cmd = ActuatorCommand::OMHOOG;
                xQueueSend(mActuator.getCommandQueue(), &a, 0);

                mState = State::WACHT_OP_BRUG_OPEN;
            }
            break;

        // ---------------------------------------------------------------------
        case BridgeEventType::BRUG_OPEN:
            if (mState == State::WACHT_OP_BRUG_OPEN)
            {
                printf("[Ophaalbrug] Brug geopend.\n");

                StoplichtCommandMsg g{};
                g.cmd = StoplichtCommand::GROEN;
                xQueueSend(mBotenStoplicht.getCommandQueue(), &g, 0);

                mState = State::OPEN;
            }
            break;

        // ---------------------------------------------------------------------
        case BridgeEventType::BRUG_DICHT:
            if (mState == State::WACHT_OP_BRUG_DICHT)
            {
                printf("[Ophaalbrug] Brug dicht → Slagboom omhoog.\n");

                SlagboomCommandMsg s{};
                s.cmd = SlagboomCommand::OPEN;
                xQueueSend(mSlagboom.getCommandQueue(), &s, 0);

                mState = State::WACHT_OP_SLAGBOOM_OPEN;
            }
            break;

        // ---------------------------------------------------------------------
        case BridgeEventType::SLAGBOOM_OPEN:
            if (mState == State::WACHT_OP_SLAGBOOM_OPEN)
            {
                printf("[Ophaalbrug] Slagboom open → autos groen.\n");

                StoplichtCommandMsg v{};
                v.cmd = StoplichtCommand::GROEN;
                xQueueSend(mVerkeersStoplicht.getCommandQueue(), &v, 0);

                mState = State::IDLE;

                // check opnieuw
                updateBrugEnStoplichten();
            }
            break;

        // ---------------------------------------------------------------------
        case BridgeEventType::NOODSTOP:
            noodstop();
            break;

        default:
            break;
    }
}

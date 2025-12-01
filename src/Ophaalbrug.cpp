#include "Ophaalbrug.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/queue.h"
}

// ---------------------------------------------------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------------------------------------------------

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
    // Queue voor events vanuit alle componenten
    mEventQueue = xQueueCreate(20, sizeof(BridgeEventMsg));
    if (!mEventQueue)
        printf("[Ophaalbrug] ERROR: kon eventqueue niet maken!\n");
}

// ---------------------------------------------------------------------------------------------------------------------
// FreeRTOS Task wrapper
// ---------------------------------------------------------------------------------------------------------------------

void Ophaalbrug::taskEntry(void* pv)
{
    auto* self = static_cast<Ophaalbrug*>(pv);
    if (self) self->taskLoop();
    vTaskDelete(nullptr);
}

// ---------------------------------------------------------------------------------------------------------------------
// Hoofd-Task Loop — ontvangt ALLE events
// ---------------------------------------------------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------------------------------------------------
// Start het openproces na SCHIP_GEDETECTEERD
// ---------------------------------------------------------------------------------------------------------------------

void Ophaalbrug::startOpenProces()
{
    printf("[Ophaalbrug] Start OPEN proces...\n");

    // 1. Schip past eronder → geen brug openen
    if (mLaatsteSchipHoogte <= mMaximaleHoogte)
    {
        printf("  | Schip past onder de brug → boten GO.\n");
        StoplichtCommandMsg sm{};
        sm.cmd = StoplichtCommand::GROEN;
        xQueueSend(mBotenStoplicht.getCommandQueue(), &sm, 0);
        return;
    }

    // 2. Verkeersstoplicht naar ROOD (verkeersstoplicht doet zelf oranje + delay!)
    {
        StoplichtCommandMsg msg{};
        msg.cmd = StoplichtCommand::ROOD;
        xQueueSend(mVerkeersStoplicht.getCommandQueue(), &msg, 0);
    }

    mState = State::WACHT_OP_VERKEERSLICHT_ROOD;
}

// ---------------------------------------------------------------------------------------------------------------------
// Start sluitproces van de brug
// ---------------------------------------------------------------------------------------------------------------------

void Ophaalbrug::startSluitProces()
{
    printf("[Ophaalbrug] Start SLUIT proces...\n");

    // 1. Boten tegenhouden
    {
        StoplichtCommandMsg msg{};
        msg.cmd = StoplichtCommand::ROOD;
        xQueueSend(mBotenStoplicht.getCommandQueue(), &msg, 0);
    }

    // 2. Brug omlaag
    {
        ActuatorCommandMsg msg{};
        msg.cmd = ActuatorCommand::OMLAAG;
        xQueueSend(mActuator.getCommandQueue(), &msg, 0);
    }

    mState = State::WACHT_OP_BRUG_DICHT;
}

// ---------------------------------------------------------------------------------------------------------------------
// NOODSTOP
// ---------------------------------------------------------------------------------------------------------------------

void Ophaalbrug::noodstop()
{
    printf("[Ophaalbrug] !!! NOODSTOP !!!\n");

    // Brugmotor stoppen
    {
        ActuatorCommandMsg msg{};
        msg.cmd = ActuatorCommand::STOP;
        xQueueSend(mActuator.getCommandQueue(), &msg, 0);
    }

    // Slagboom sluiten
    {
        SlagboomCommandMsg msg{};
        msg.cmd = SlagboomCommand::CLOSE;
        xQueueSend(mSlagboom.getCommandQueue(), &msg, 0);
    }

    // Beide stoplichten rood
    {
        StoplichtCommandMsg msg{};
        msg.cmd = StoplichtCommand::ROOD;
        xQueueSend(mVerkeersStoplicht.getCommandQueue(), &msg, 0);
        xQueueSend(mBotenStoplicht.getCommandQueue(),    &msg, 0);
    }

    mState = State::IDLE;
}

// ---------------------------------------------------------------------------------------------------------------------
// Event Handler
// ---------------------------------------------------------------------------------------------------------------------

void Ophaalbrug::onEvent(const BridgeEventMsg& msg)
{
    switch (msg.type)
    {
        case BridgeEventType::SCHIP_GEDETECTEERD:
            mLaatsteSchipHoogte  = msg.schipHoogte;
            mLaatsteSchipBreedte = msg.schipBreedte;

            if (mState == State::IDLE)
                startOpenProces();
            break;

        case BridgeEventType::VERKEERSLICHT_ROOD:
            if (mState == State::WACHT_OP_VERKEERSLICHT_ROOD)
            {
                printf("[Ophaalbrug] Verkeerslicht is ROOD → slagboom sluiten.\n");

                SlagboomCommandMsg s{};
                s.cmd = SlagboomCommand::CLOSE;
                xQueueSend(mSlagboom.getCommandQueue(), &s, 0);

                mState = State::WACHT_OP_SLAGBOOM_DICHT;
            }
            break;

        case BridgeEventType::SLAGBOOM_DICHT:
            if (mState == State::WACHT_OP_SLAGBOOM_DICHT)
            {
                printf("[Ophaalbrug] Slagboom dicht → brug gaat omhoog.\n");

                ActuatorCommandMsg a{};
                a.cmd = ActuatorCommand::OMHOOG;
                xQueueSend(mActuator.getCommandQueue(), &a, 0);

                mState = State::WACHT_OP_BRUG_OPEN;
            }
            break;

        case BridgeEventType::BRUG_OPEN:
            if (mState == State::WACHT_OP_BRUG_OPEN)
            {
                printf("[Ophaalbrug] Brug open → boten mogen door (GROEN).\n");

                StoplichtCommandMsg b{};
                b.cmd = StoplichtCommand::GROEN;
                xQueueSend(mBotenStoplicht.getCommandQueue(), &b, 0);

                mState = State::OPEN;
            }
            break;

        case BridgeEventType::GEEN_WACHTENDE_SCHEPEN:
            if (mState == State::OPEN)
                startSluitProces();
            break;

        case BridgeEventType::BRUG_DICHT:
            if (mState == State::WACHT_OP_BRUG_DICHT)
            {
                printf("[Ophaalbrug] Brug dicht → slagboom openen.\n");

                SlagboomCommandMsg s{};
                s.cmd = SlagboomCommand::OPEN;
                xQueueSend(mSlagboom.getCommandQueue(), &s, 0);

                mState = State::WACHT_OP_SLAGBOOM_OPEN;
            }
            break;

        case BridgeEventType::SLAGBOOM_OPEN:
            if (mState == State::WACHT_OP_SLAGBOOM_OPEN)
            {
                printf("[Ophaalbrug] Slagboom open → verkeer groen.\n");

                StoplichtCommandMsg v{};
                v.cmd = StoplichtCommand::GROEN;
                xQueueSend(mVerkeersStoplicht.getCommandQueue(), &v, 0);

                mState = State::IDLE;
            }
            break;

        case BridgeEventType::NOODSTOP:
            noodstop();
            break;

        default:
            break;
    }
}

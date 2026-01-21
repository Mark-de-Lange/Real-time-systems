#include "BotenStoplicht.hpp"
#include "Commands.hpp"
#include <cstdio>

extern "C" {
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "freertos/queue.h"
}

BotenStoplicht::BotenStoplicht(int pinR, int pinG)
: mRood(false),
  mGroen(false),
  mPinR(pinR),
  mPinG(pinG),
  mPrevDoorlaat(false),
  mPrevTegenhouden(true)
{
    gpio_config_t cfg{};
    cfg.intr_type    = GPIO_INTR_DISABLE;
    cfg.mode         = GPIO_MODE_OUTPUT;
    cfg.pin_bit_mask = (1ULL << mPinR) | (1ULL << mPinG);
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&cfg);

    gpio_set_level((gpio_num_t)mPinR, 1);
    gpio_set_level((gpio_num_t)mPinG, 0);

    printf("[BotenStoplicht] Init R=%d G=%d\n", mPinR, mPinG);

    mCommandQueue = xQueueCreate(5, sizeof(StoplichtCommandMsg));
    if (mCommandQueue == nullptr) {
        printf("[BotenStoplicht] ERROR: kon command queue niet maken!\n");
    }
}

void BotenStoplicht::FaseHandler()
{
    mRood  = false;
    mGroen = false;

    if (mTegenhouden)
        mRood = true;
    else if (mDoorlaat)
        mGroen = true;

    gpio_set_level((gpio_num_t)mPinR, mRood);
    gpio_set_level((gpio_num_t)mPinG, mGroen);

    printf("[BotenStoplicht] Fase: rood=%d groen=%d\n", mRood, mGroen);
}

int BotenStoplicht::GetFase() const
{
    if (mRood)  return 0;
    if (mGroen) return 1;
    return -1;
}

void BotenStoplicht::SetKleur(int kleur)
{
    // 0 = rood, 1 = groen
    switch (kleur) {
        case 0:
            SetTegenhouden(true);
            SetDoorlaat(false);
            break;

        case 1:
        default:
            SetDoorlaat(true);
            SetTegenhouden(false);
            break;
    }
}

void BotenStoplicht::BotenStoplichtTask(void* pv)
{
    BotenStoplicht* self = static_cast<BotenStoplicht*>(pv);
    StoplichtCommandMsg cmd{};

    for (;;) 
    {

        if (self->mCommandQueue != nullptr &&
            xQueueReceive(self->mCommandQueue, &cmd, portMAX_DELAY) == pdTRUE)
        {
            printf("[BotenStoplicht] Command ontvangen: %d\n", static_cast<int>(cmd.cmd));
            switch (cmd.cmd)
            {
                case StoplichtCommand::ROOD:
                    self->mDoorlaat = false;
                    self->mTegenhouden = true;
                    break;
                case StoplichtCommand::GROEN:
                    self->mDoorlaat = true;
                    self->mTegenhouden = false;
                    break;
                default:
                    break;
            }

            self->FaseHandler();

            self->mPrevDoorlaat    = self->mDoorlaat;
            self->mPrevTegenhouden = self->mTegenhouden;
        }
    }
}

void BotenStoplicht::startTask(const char* name, UBaseType_t prio, uint32_t stack)
{
    xTaskCreate(
        BotenStoplichtTask,
        name,
        stack,
        this,
        prio,
        &mTask
    );
}

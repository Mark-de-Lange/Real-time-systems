#include "BotenStoplicht.hpp"
#include <cstdio>

BotenStoplicht::BotenStoplicht(int pinR, int pinG)
: mRood(false),
  mGroen(false),
  mPinR(pinR),
  mPinG(pinG)
{
    gpio_config_t io_conf{};
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << mPinR) | (1ULL << mPinG);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level((gpio_num_t)mPinR, 0);
    gpio_set_level((gpio_num_t)mPinG, 0);
}

void BotenStoplicht::FaseHandler()
{
    // Zelfde logica: tegenhouden = rood, doorlaat = groen
    mRood  = false;
    mGroen = false;

    if (mTegenhouden) {
        mRood = true;
    } else if (mDoorlaat) {
        mGroen = true;
    } else {
        // alles uit
    }

    gpio_set_level((gpio_num_t)mPinR, mRood  ? 1 : 0);
    gpio_set_level((gpio_num_t)mPinG, mGroen ? 1 : 0);

    std::printf("[BotenStoplicht] Fase: rood=%d, groen=%d\n",
                (int)mRood, (int)mGroen);
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
            break;
        case 1:
        default:
            SetDoorlaat(true);
            break;
    }
}

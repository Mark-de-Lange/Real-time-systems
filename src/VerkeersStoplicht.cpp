#include "VerkeersStoplicht.hpp"
#include <cstdio>

VerkeersStoplicht::VerkeersStoplicht(int pinR, int pinO, int pinG)
: mRood(false),
  mOranje(false),
  mGroen(false),
  mPinR(pinR),
  mPinO(pinO),
  mPinG(pinG)
{
    // GPIO's als output configureren
    gpio_config_t io_conf{};
    io_conf.intr_type    = GPIO_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << mPinR) | (1ULL << mPinO) | (1ULL << mPinG);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Alles uit bij start
    gpio_set_level((gpio_num_t)mPinR, 0);
    gpio_set_level((gpio_num_t)mPinO, 0);
    gpio_set_level((gpio_num_t)mPinG, 0);
}

void VerkeersStoplicht::FaseHandler()
{
    // Bepaal logische kleuren op basis van mDoorlaat / mTegenhouden
    // (zoals besproken)
    mRood   = false;
    mOranje = false;
    mGroen  = false;

    if (mTegenhouden) {
        mRood = true;   // rood = verkeer stoppen
    } else if (mDoorlaat) {
        mGroen = true;  // groen = verkeer doorlaten
    } else {
        // "tussenfase" → oranje
        mOranje = true;
    }

    // Nu de GPIO's zetten
    gpio_set_level((gpio_num_t)mPinR, mRood   ? 1 : 0);
    gpio_set_level((gpio_num_t)mPinO, mOranje ? 1 : 0);
    gpio_set_level((gpio_num_t)mPinG, mGroen  ? 1 : 0);

    std::printf("[VerkeersStoplicht] Fase: rood=%d, oranje=%d, groen=%d\n",
                (int)mRood, (int)mOranje, (int)mGroen);
}

int VerkeersStoplicht::GetFase() const
{
    if (mRood)   return 0;
    if (mOranje) return 1;
    if (mGroen)  return 2;
    return -1; // alles uit
}

void VerkeersStoplicht::SetKleur(int kleur)
{
    // Maak het simpel en laat dit via de basisklasse lopen:
    // 0 = rood  -> tegenhouden
    // 1 = oranje-> tussenfase (geen doorlaat, geen tegenhouden)
    // 2 = groen -> doorlaat
    switch (kleur) {
        case 0:
            SetTegenhouden(true);
            break;
        case 2:
            SetDoorlaat(true);
            break;
        case 1:
        default:
            // oranje: beide false -> FaseHandler zet dan oranje aan
            mDoorlaat     = false;
            mTegenhouden  = false;
            if (mTask != nullptr) {
                xTaskNotifyGive(mTask);
            }
            break;
    }
}

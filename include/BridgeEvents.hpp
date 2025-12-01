#ifndef BRIDGE_EVENTS_HPP
#define BRIDGE_EVENTS_HPP

#include <cstdint>

// Alle soorten events die naar de Ophaalbrug gaan
enum class BridgeEventType {
    SCHIP_GEDETECTEERD,
    GEEN_WACHTENDE_SCHEPEN,
    SLAGBOOM_DICHT,
    SLAGBOOM_OPEN,
    BRUG_OPEN,
    BRUG_DICHT,
    NOODSTOP
};

// Wat er in de queue wordt verstuurd
struct BridgeEventMsg {
    BridgeEventType type;
    int schipHoogte;   // alleen gebruikt bij SCHIP_GEDETECTEERD
    int schipBreedte;  // idem
};

#endif // BRIDGE_EVENTS_HPP

#ifndef COMMANDS_HPP
#define COMMANDS_HPP

// ---------------------------------------------------------------------
// Uniform stoplicht commando — geldend voor ALLE stoplichten
// ---------------------------------------------------------------------
enum class StoplichtCommand {
    NONE = 0,
    GROEN,
    ORANJE,
    ROOD
};

// Stoplicht message
struct StoplichtCommandMsg {
    StoplichtCommand cmd;
};

// ---------------------------------------------------------------------
// Actuator (brugmotor)
// ---------------------------------------------------------------------
enum class ActuatorCommand {
    NONE = 0,
    OMHOOG,
    OMLAAG,
    STOP
};

struct ActuatorCommandMsg {
    ActuatorCommand cmd;
};

// ---------------------------------------------------------------------
// Slagboom
// ---------------------------------------------------------------------
enum class SlagboomCommand {
    NONE = 0,
    OPEN,
    CLOSE
};

struct SlagboomCommandMsg {
    SlagboomCommand cmd;
};

#endif

/*
 * state.h — Top-level finite state machine
 */
#ifndef STATE_H
#define STATE_H

enum class State : uint8_t {
    BOOT,            // power-on, init hardware
    CONNECTING_WIFI, // captive portal or connecting
    SYNCING_TIME,    // NTP + RTC
    IDLE,            // ready, waiting for finger
    WAITING_FINGER,  // prompted, polling sensor
    CAPTURING,       // image acquisition in progress
    MATCHING,        // sensor searching template DB
    SHOWING_RESULT,  // displaying match for ~2s
    ENROLLING,       // admin-triggered enrollment mode
    FLUSHING_QUEUE,  // pushing pending offline events
    ERROR_RECOVERY   // sensor comms lost, retrying
};

struct AppContext {
    State        state      = State::BOOT;
    unsigned long stateMs   = 0;
    uint16_t      lastMatchId  = 0;
    uint16_t      lastMatchScore = 0;
    char          lastMatchName[40] = {0};
    uint8_t       errorCount = 0;
    bool          wifiUp     = false;
    bool          ntpUp      = false;
    bool          queueDirty = false;
};

extern AppContext ctx;

inline const char* stateName(State s) {
    switch (s) {
        case State::BOOT:            return "BOOT";
        case State::CONNECTING_WIFI: return "WIFI";
        case State::SYNCING_TIME:    return "NTP";
        case State::IDLE:            return "IDLE";
        case State::WAITING_FINGER:  return "WAIT";
        case State::CAPTURING:       return "CAPTURE";
        case State::MATCHING:        return "MATCH";
        case State::SHOWING_RESULT:  return "RESULT";
        case State::ENROLLING:       return "ENROLL";
        case State::FLUSHING_QUEUE:  return "FLUSH";
        case State::ERROR_RECOVERY:  return "ERROR";
    }
    return "?";
}

#endif // STATE_H

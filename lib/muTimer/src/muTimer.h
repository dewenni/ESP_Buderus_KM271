/*
    Name: muTimer Library
    Author: Michael Uray
    E-Mail: mu@spamfence.net
    Source Repository: https://github.com/MichaelUray/muTimer
    Copyright (C): Michael Uray / Graz / Austria
    Licence: MIT, check LICENSE file (https://github.com/MichaelUray/muTimer/blob/main/LICENSE) which is included with the source
*/

#ifndef mTimer_h
#define mTimer_h
#include "Arduino.h"

class muTimer
{
public:
    // constructor
    muTimer(void);

    // ------
    // Delays
    // ------

    // delay on and off
    bool delayOnOff(bool input, uint32_t delayTimeSwitchOn, uint32_t delayTimeSwitchOff);

    // delay on
    bool delayOn(bool input, uint32_t delayTimeSwitchOn);

    // delay off
    bool delayOff(bool input, uint32_t delayTimeSwitchOff);

    // delay on and off with trigger output
    // sets the output to 0 once if the delayTimeSwitchOff elapsed and if the output of delayOnOff() would go to 0
    // sets the output to 1 once if the delayTimeSwitchOn elapsed and if the output of delayOnOff() would go to 1
    // sets the output to 2 if the delay time is running
    byte delayOnOffTrigger(bool input, uint32_t delayTimeSwitchOn, uint32_t delayTimeSwitchOff);

    // delay on with trigger output - gets true only once if the time is elapsed
    bool delayOnTrigger(bool input, uint32_t delayTimeSwitchOn);

    // delay off with trigger output - gets true only once if the time is elapsed
    bool delayOffTrigger(bool input, uint32_t delayTimeSwitchOff);

    // -------------
    // Delay Control
    // -------------

    // restarts the time from 0 and sets output != input at next delay function call
    void delayReset(void);

    // ends the current running time interval and sets output == input at next delay function call
    void delayElapse(void);

    // -----------------
    // Delay Information
    // -----------------

    // returns true if delay is still running
    bool delayIsRunning(void);

    // -----
    // Cycle
    // -----

    // cycle on/off, sets the output between on and off by the given time intervals, could get used to create a flashing LED
    bool cycleOnOff(uint32_t onTime, uint32_t offTime);

    // cycle on and off with trigger
    // sets the output to 0 once if the onTime elapsed and if the output of delayOnOffCycle() would go to 0
    // sets the output to 1 once if the offTime elapsed and if the output of delayOnOffCycle() would go to 1
    // sets the output to 2 if the time between cycles is running
    byte cycleOnOffTrigger( uint32_t onTime, uint32_t offTime);

    // triggers the output periodically once by the given cycleTime
    bool cycleTrigger(uint32_t cycleTime);

    // -------------
    // Cycle control
    // -------------

    // cycle reset to output off, allows to synchronize the cycle with other timings
    void cycleResetToOff(void);

    // cycle reset to output on, allows to synchronize the cycle with other timings
    void cycleResetToOn(void);

    // -------------------
    // General Information
    // -------------------

    // returns the time elapsed since start
    uint32_t getTimeElapsed(void);

    // -------------
    // Configuration
    // -------------

    // set time base to ms (default)
    void setTimeBaseToMs(void);

    // set time base to us
    void setTimeBaseToUs(void);

private:
    // memorize input status
    bool _input_M;

    // output status
    bool _output;

    // time base in us
    bool _usTimeBase;

    // start time of timer/delay function
    uint32_t _startTime;

    // returns the current time in ms or us since start of the MCU
    uint32_t getCurrentTime(void);
};

#endif /* mTimer_h */
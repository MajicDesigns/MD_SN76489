/*
MD_SN76489 - Library for using a SN74689 sound generator

See header file for copyright and licensing comments.
*/
#include "MD_SN76489.h"

/**
* \file
* \brief Implements class definition and general methods
*/

#ifndef LIBDEBUG
#define LIBDEBUG 0    ///< Control debugging output. See \ref pageCompileSwitch
#endif

#if LIBDEBUG
#define DEBUGS(s) { Serial.print(F(s)); }
#define DEBUGX(s, v) { Serial.print(F(s)); Serial.print(v, HEX); }
#define DEBUG(s, v) { Serial.print(F(s)); Serial.print(v); }
#else
#define DEBUGS(s)
#define DEBUGX(s, v)
#define DEBUG(s, v)
#endif

// Class methods
MD_SN76489::MD_SN76489(bool clock): _clock(clock)
{
  _adsrDefault.invert = false;    // Normal non-inverted curve
  _adsrDefault.Ta = 40;           // Time for attack curve to reach Vmax
  _adsrDefault.Td = 60;           // Time for decay curve to reach Vs
  _adsrDefault.deltaVs = 3;       // Sustain volume delta from setpoint
  _adsrDefault.Tr = 75;           // Time for Release curve to reach 0 volume
}

void MD_SN76489::begin(void)
{
  // Start the clock if configured
  if (_clock)
    startClock();

  for (uint8_t i = 0; i < MAX_CHANNELS; i++)
  {
    C[i].state = IDLE;
    C[i].adsr = &_adsrDefault;
    C[i].volSP = VOL_MAX;   // all setpoints to max
    setCVolume(i, VOL_OFF); // all currents to off and write to device
  }
}

bool MD_SN76489::setADSR(adsrEnvelope_t* padsr)
{
  bool b = true;

  for (uint8_t c = 0; c < MAX_CHANNELS; c++)
    b &= setADSR(c, padsr);

  return(b);
}

bool MD_SN76489::setADSR(uint8_t chan, adsrEnvelope_t* padsr)
{
  bool b = false;

  if (chan < MAX_CHANNELS)
  {
    if (isIdle(chan))
    {
      if (padsr == nullptr)
        C[chan].adsr = &_adsrDefault;
      else
        C[chan].adsr = padsr;
      b = true;
    }
  }

  return(b);
}

bool MD_SN76489::isIdle(uint8_t chan)
{
  bool b = false;

  if (chan < MAX_CHANNELS)
    b = (C[chan].state == IDLE);

  return(b);
}

uint16_t MD_SN76489::calcTs(uint8_t chan, uint16_t duration)
// work out what the Vs time should be for this note
// if it is zero, return 0.
// if it works out negative, return 1 (ie, not zero)
// otherwise return the calculated value
{
  if (duration != 0)
  {
    // work out what the Vs time should be for this note
    if (duration < C[chan].adsr->Ta + C[chan].adsr->Td + C[chan].adsr->Tr)
      duration = 1;   
    else
      duration -= C[chan].adsr->Ta + C[chan].adsr->Td + C[chan].adsr->Tr;
  }

  return(duration);
}

void MD_SN76489::tone(byte chan, uint16_t freq, uint8_t volume, uint16_t duration)
// play a tone without ADSR
{
  DEBUG("\ntone C", chan);
  DEBUG(" F", freq);
  if (chan < MAX_CHANNELS - 1)   // noise channel not valid for this
  {
    DEBUGS(" note ");

    if (freq != 0)
    {
      DEBUGS("on");
      C[chan].frequency = freq;
      C[chan].volSP = C[chan].volCV = volume;
      C[chan].duration = duration;
      C[chan].playTone = true;
      C[chan].state = TONE_ON;
    }
    else
    {
      DEBUGS("off");
      C[chan].state = IDLE;
    }
  }
}

void MD_SN76489::note(byte chan, uint16_t freq, uint8_t volume, uint16_t duration)
// queue a note to be played with ADSR
{
  DEBUG("\nnote C", chan);
  DEBUG(" F", freq);
  if (chan < MAX_CHANNELS - 1)   // noise channel not valid for this
  {
    DEBUGS(" note ");

    if (freq != 0)
    {
      DEBUGS("on");
      C[chan].frequency = freq;
      C[chan].volSP = C[chan].volCV = volume;
      C[chan].duration = calcTs(chan, duration);
      C[chan].playTone = false;
      C[chan].state = NOTE_ON;
    }
    else
    {
      DEBUGS("off");
      C[chan].state = NOTE_OFF;
    }
  }
}

void MD_SN76489::noise(noiseType_t noise, uint8_t volume, uint16_t duration)
// queue a noise to be played with ADSR
{
  {
    DEBUG("\nnoise ", noise);

    if (noise != NOISE_OFF)
    {
      DEBUGS("on");
      C[NOISE_CHANNEL].frequency = noise;
      C[NOISE_CHANNEL].volSP = C[NOISE_CHANNEL].volCV = volume;
      C[NOISE_CHANNEL].duration = calcTs(NOISE_CHANNEL, duration);
      C[NOISE_CHANNEL].state = NOISE_ON;
    }
    else
    {
      DEBUGS("off");
      C[NOISE_CHANNEL].state = NOTE_OFF;
    }
  }
}

void MD_SN76489::play(void)
{
  for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
  {
    switch (C[chan].state)
    {
    case IDLE:    // doing nothing, just make sure the volume is turned off
    {
      if (C[chan].volCV != VOL_OFF)
        setCVolume(chan, VOL_OFF);
    }
    break;

    case NOISE_ON:  // set up the hardware to play this noise with ADSR
    case NOTE_ON:   // set up the hardware to play this note with ADSR
    {
      DEBUGS("\n->NOTE/NOISE_ON");

      if (C[chan].state == NOTE_ON)
        setFrequency(chan, C[chan].frequency);   // set channel frequency
      else
        setNoise((noiseType_t)(C[chan].frequency));

      // set timing parameters for ATTACK phase
      C[chan].timeBase = millis();
      C[chan].timeStep = (C[chan].adsr->Ta / C[chan].volSP);

      // set inital playing volume and volume step direction
      setCVolume(chan, C[chan].adsr->invert ? C[chan].volSP : 0);
      C[chan].volumeStep = C[chan].adsr->invert ? -1 : 1;

      DEBUGS("\n->NOTE/NOISE_ON to ATTACK");
      C[chan].state = ATTACK;
    }
    break;

    case TONE_ON:   // set up the hardware to play this note without
    {
      DEBUGS("\n->TONE_ON");

      // set channel frequency
      setFrequency(chan, C[chan].frequency);

      // set timing parameters for SUSTAIN phase
      C[chan].timeBase = millis();

      // set inital playing volume
      setCVolume(chan, C[chan].volSP);

      DEBUGS("\n->TONE_ON to SUSTAIN");
      C[chan].state = SUSTAIN;
    }
    break;

    case ATTACK:
    {
      // check if enough time has passed to do something
      if (millis() - C[chan].timeBase >= C[chan].timeStep)
      {
        // if the current level was the end of the interval
        if ((C[chan].adsr->invert && C[chan].volCV == 0) ||
            (!C[chan].adsr->invert && C[chan].volCV == C[chan].volSP))
        {
          // set timing parameters for DECAY phase
          C[chan].timeBase = millis();
          C[chan].timeStep = C[chan].adsr->Td / C[chan].adsr->deltaVs;

          // reverse volume step direction from current one
          C[chan].volumeStep *= -1;

          DEBUGS("\n->ATTACK to DECAY");
          C[chan].state = DECAY;
        }
        else
        {
          DEBUG("\n--ATTACK Volume delta ", C[chan].volumeStep);
          // still in ATTACK, just set the volume to the new level
          setCVolume(chan, C[chan].volCV + C[chan].volumeStep);
          C[chan].timeBase += C[chan].timeStep;
        }
      }
    }
    break;

    case DECAY:
    {
      // check if enough time has passed to do something
      if (millis() - C[chan].timeBase >= C[chan].timeStep)
      {
        int8_t volEnd = (C[chan].volSP - C[chan].adsr->deltaVs < 0) ? 0 : (C[chan].volSP - C[chan].adsr->deltaVs);

        // if the current level was the end of the interval
        if (C[chan].volCV == volEnd)
        {
          C[chan].timeBase = millis();
          C[chan].state = SUSTAIN;
          DEBUG("\n->DECAY to SUSTAIN: duration ", C[chan].duration);
        }
        else
        {
          DEBUG("\n--DECAY Volume delta ", C[chan].volumeStep);
          // still in DECAY, just set the volume to the new level
          setCVolume(chan, C[chan].volCV + C[chan].volumeStep);
          C[chan].timeBase += C[chan].timeStep;
        }
      }
    }
    break;

    case SUSTAIN:
    {
      // if configured, wait for the duration to expire, otherwise
      // do nothing but keep playing the same note at current volume
      if (C[chan].duration != 0)
      {
        if (millis() - C[chan].timeBase >= C[chan].duration)
          C[chan].state = C[chan].playTone ? IDLE : NOTE_OFF;
      }
    }
    break;

    case NOTE_OFF:
    {
      DEBUGS("\n->NOTE_OFF");
      // set timing parameters for RELEASE phase
      C[chan].timeBase = millis();
      C[chan].timeStep = C[chan].adsr->Tr / (C[chan].volSP - C[chan].adsr->deltaVs);

      // volume step direction remains the same as for previous DECAY
      // but we set this explicitely as NOTE_OFF can happen anytime,
      // before we finish ATTACK and timeStep is actually set.
      C[chan].volumeStep = C[chan].adsr->invert ? 1 : -1;

      DEBUGS("\n->NOTE_OFF to RELEASE");
      C[chan].state = RELEASE;
    }
    break;

    case RELEASE:
    {
      // check if enough time has passed to do something
      if (millis() - C[chan].timeBase >= C[chan].timeStep)
      {
        // if the current level was the end of the interval
        if ((!C[chan].adsr->invert && C[chan].volCV == 0) ||
            (C[chan].adsr->invert && C[chan].volCV == C[chan].volSP))
        {
          DEBUGS("\n->RELEASE to IDLE");
          setCVolume(chan, VOL_OFF);
          C[chan].state = IDLE;
        }
        else
        {
          DEBUG("\n--RELEASE Volume delta ", C[chan].volumeStep);
          // still in RELEASE, just set the volume to the new level
          setCVolume(chan, C[chan].volCV + C[chan].volumeStep);
          C[chan].timeBase += C[chan].timeStep;
        }
      }
    }
    break;

    default:
      C[chan].state = IDLE;
      break;
    }
  }
}

void MD_SN76489::setCVolume(uint8_t chan, uint8_t v)
// Set the volume current value for channel and remember the setting
// Application values are 0-15 for min to max. Attenuator values
// are the complement of this (15-0).
{
  DEBUG("\nsetCVolume C", chan);
  DEBUG(" V", v);

  if (v > VOL_MAX) v = VOL_MAX;
  uint8_t cmd = LATCH_CMD | (chan << 5) | TYPE_VOL | ((0xf - v) & DATA1_MASK);
  DEBUGX(" : 0x", cmd);
  send(cmd);
  C[chan].volCV = v;
}

void MD_SN76489::setVolume(uint8_t chan, uint8_t v)
// Set the volume set point for channel and remember the setting
// Application values are 0-15 for min to max. Attenuator values
// are the complement of this (15-0).
{
  if (chan < MAX_CHANNELS)
  {
    if (v > VOL_MAX) v = VOL_MAX;
    setCVolume(chan, v);
    C[chan].volSP = v;
  }
}

void MD_SN76489::setVolume(uint8_t v)
// Set the same volume set point for all channels
{
  for (int8_t i = 0; i < MAX_CHANNELS; i++)
    setVolume(i, v);
}

void MD_SN76489::setFrequency(uint8_t chan, uint16_t freq)
// Calculate register values and set them
{
  DEBUG("\nsetFrequency C", chan);
  DEBUG(" F", freq);

  if (chan < MAX_CHANNELS - 2)    // last channel only does noise
  {
    uint16_t n = CLOCK_HZ / ((uint32_t)freq << 5);  // <<5 same as *32

    DEBUGX(" : 0x", n);
    // Send frequency data in two parts of the frequency
    send(LATCH_CMD | (chan << 5) | TYPE_TONE | (n & DATA1_MASK));
    send(DATA_CMD | ((n >> 4) & DATA2_MASK));
  }
}

void MD_SN76489::setNoise(noiseType_t noise)
// Set the noise channel parameters
{
  if (noise != NOISE_OFF)
    send(LATCH_CMD | (NOISE_CHANNEL << 5) | noise);
  else
    setVolume(NOISE_CHANNEL, 0);
}

void MD_SN76489::send(uint8_t data)
{
  DEBUGX("\nVIRTUAL send of byte 0x", data);
}

void MD_SN76489::startClock(void)
// HARDWARE DEPENDENT CODE!!
{
  DEBUGS("\nstartClock");

#if defined(__AVR_ATmega328P__)
  // Uno
  /*
  Using Timer2, creates a 4Mhz PWM signal 50% duty cycle on pin 3 of Arduino Uno.

  This tells the chip MCU to:
  - Enable Fast PWM Mode (WGM22, WGM21, WGM20).
  - Don't scale the clock signal - keep it at 16 MHz (CS22, CS21, CS20).
  - When the counter TCNT2 equals OCR2A, start over from 0 (COM2B1, COM2B0).
  - When the counter TCNT2 equals OCR2B, set pin3 to 0; when counter TCNT2 equals 0, set OCR2B to 1.

  OCR2A = 3: The counter will start over at 3. So it'll count 0, 1, 2, 3, 0, 1, etc
  OCR2B = 1: Pin3 will toggle off at 1, and toggle back on at 0.
    
  Since the pin makes a complete cycle every four clock ticks, the resulting 
  PWM frequency is 4 MHz (16Mhz/4).
  */
  #define CLOCK_PIN 3  // Pin 3 will be the 4MHz signal.
  pinMode(CLOCK_PIN, OUTPUT);

  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(WGM22) | _BV(CS20);     
  OCR2A = 3;
  OCR2B = 1;

#define _STARTCLOCK_
#endif

#if defined(__AVR_ATmega32U4__) && defined(CORE_TEENSY)
  // Teensy 2.0
#define CLOCK_PIN 14
  // Make pin 14 be a 4MHz signal.
  pinMode(CLOCK_PIN, OUTPUT);
  TCCR1A = 0x43;  // 01000011
  TCCR1B = 0x19;  // 00011001
  OCR1A = 1;

#define _STARTCLOCK_
#endif

#if defined(__AVR_ATmega32U4__)
  // Leonardo
#endif

#if defined(__AVR_ATmega168__)
  // Arduino Duemilanove, Diecimila, and NG
#endif

#if defined(__AVR_ATmega1280__)
  // Mega
#endif

#if defined(__AVR_ATmega2560__)
  // Mega
#endif

#if defined (__AVR_ATtiny84__) || defined(__AVR_ATtiny44__)
#define CLOCK_PIN 6
  /*
   4Mhz with 50% duty cycle (8Mhz CPU clock)
   WGM10, WGM11, WGM12, WGM13: Fast PWM TOP=OCR1A
   CS10: No prescaler
   COM1A0: Toggle Pin 6 on TOP
  */
  TCCR1A = _BV(COM1A0) | _BV(WGM11) | _BV(WGM10);
  TCCR1B = _BV(CS10) | _BV(WGM12) | _BV(WGM13);
  OCR1A = 0;

#defdine _STARTCLOCK_
#endif

#if defined(__SAM3X8E__)
  // DUE
#endif

#if defined (__AVR_AT90USB162__)
  // Teensy 1.0
#endif

#if defined(__MK20DX128__) || defined(__MK20DX256__)
  // Teensy 3.0 and 3.1
#endif

#if defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB1286__)
  // Teensy++ 1.0 and 2.0
#endif

#ifndef _STARTCLOCK_
  #warning 4MHz CLOCK GENERATION UNDEFINED FOR THIS MCU ARCHITECTURE
#endif
}


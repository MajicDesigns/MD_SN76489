
#include "MD_SN76489.h"

/**
* \file
* \brief Implements class definition and general methods
*/

#ifndef CLOCK_GENERATOR
#define CLOCK_GENERATOR 1
#endif

#ifndef LIBDEBUG
#define LIBDEBUG 0
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
MD_SN74689::MD_SN74689(const uint8_t *D, uint8_t WE, bool clock):
  _D(D), _WE(WE), _clock(clock)
{
  _adsrDefault.invert = false;           // Normal non-inverted curve
  _adsrDefault.Vmax = VOL_MAX;           // Attack volume setting
  _adsrDefault.Vs = (VOL_MAX * 2) / 3;   // Sustain volume setting
  _adsrDefault.Ta = 200;                 // Time for attack curve to reach Vmax
  _adsrDefault.Td = 300;                 // Time for decay curve to reach Vs
  _adsrDefault.Tr = 200;                 // Time for Release curve to reach 0 volume
}

MD_SN74689::~MD_SN74689(void)
{
}

void MD_SN74689::begin()
{
  // Start the clock if configured
  if (_clock)
    startClock();

  // Set all pins to outputs
  for (int8_t i = 0; i < DATA_BITS; i++)
    pinMode(_D[i], OUTPUT);
  pinMode(_WE, OUTPUT);

  for (uint8_t i = 0; i < MAX_CHANNELS; i++)
  {
    C[i].state = IDLE;
    C[i].adsr = &_adsrDefault;
  }

  setVolume(VOL_OFF);
}

void MD_SN74689::startClock(void)
// HARDWARE DEPENDENT CODE!!
{
#if CLOCK_GENERATOR
  DEBUGS("\nstartClock");

/*#if defined(TEENSY20)
#define CLOCK_PIN 14
  // Make pin 14 be a 4MHz signal.
  pinMode(CLOCK_PIN, OUTPUT);
  TCCR1A = 0x43;  // 01000011 
  TCCR1B = 0x19;  // 00011001
  OCR1A = 1;
#elif defined(ARDUINO_AVR)
*/#define CLOCK_PIN 3
  // Make pin 3 be a 4MHz signal.
  pinMode(CLOCK_PIN, OUTPUT);
  TCCR2A = 0x23;  // 00100011
  TCCR2B = 0x09;  // 00001001
  OCR2A = 3;
  OCR2B = 1;
/*#else
#error 4MHz stuff not defined.
#endif
*/
#endif  // CLOCK_GENERATOR
}

void MD_SN74689::setVolume(uint8_t v)
// Set the same volume for all channels
{
  for (int8_t i = 0; i < MAX_CHANNELS; i++)
    setVolume(i, v);
}

void MD_SN74689::setVolume(uint8_t chan, uint8_t v)
// Send volume message and remember the setting
// Application values are 0-15 for min to max. Attenuator values
// are the complement of this (15-0).
{
  DEBUG("\nsetVolume C", chan);
  DEBUG(" V", v);

  if (chan < MAX_CHANNELS)
  {
    uint8_t cmd = LATCH_CMD | (chan << 5) | TYPE_VOL | ((0xf - v) & DATA1_MASK);
    DEBUGX(" : 0x", cmd);
    send(cmd);
    C[chan].volume = v;
  }
} 

void MD_SN74689::setADSR(uint8_t chan, adsrEnvelope_t* padsr)
{
  if (chan < MAX_CHANNELS)
  {
    if (isIdle(chan))
    {
      if (padsr == nullptr)
        C[chan].adsr = &_adsrDefault;
      else
        C[chan].adsr = padsr;
    }
  }
}

bool MD_SN74689::isIdle(uint8_t chan)
{
  bool b = false;

  if (chan < MAX_CHANNELS)
    b = (C[chan].state == IDLE);

  return(b);
}

void MD_SN74689::play(byte chan, uint32_t freq)
// queue a note to be played
{
  DEBUG("\nplay C", chan);
  DEBUG(" F", freq);
  if (chan < MAX_CHANNELS - 1)   // noise channel not valid for this
  {
    DEBUGS(" note ");

    if (freq != 0)
    {
      DEBUGS("on");
      C[chan].frequency = freq;
      C[chan].state = NOTE_ON;
    }
    else
    {
      DEBUGS("off");
      C[chan].state = NOTE_OFF;
    }
  }
}

void MD_SN74689::run(void)
{
  for (uint8_t chan = 0; chan < MAX_CHANNELS; chan++)
  {
    switch (C[chan].state)
    {
    case IDLE:
    {
      if (C[chan].volume != 0)
        setVolume(chan, 0);
    }
    break;

    case NOTE_ON:   // set up the hardware to play this note
    {
      DEBUGS("\n->NOTE_ON");
      uint16_t n = (CLOCK_HZ / (32 * C[chan].frequency));    // calculation from the data sheet

      // Send frequency data in two parts of the frequency
      send(LATCH_CMD | (chan << 5) | TYPE_TONE | (n & DATA1_MASK));
      send(DATA_CMD | ((n >> 4) & DATA2_MASK));

      // set timing parameters for ATTACK phase
      C[chan].timeBase = millis();
      C[chan].timeStep = (C[chan].adsr->Ta / C[chan].adsr->Vmax);

      // set inital playing volume and volume step direction
      setVolume(chan, C[chan].adsr->invert ? C[chan].adsr->Vmax : 0);
      C[chan].volumeStep = C[chan].adsr->invert ? -1 : 1;

      DEBUGS("\n->NOTE_ON to ATTACK");
      C[chan].state = ATTACK;
    }
    break;

    case ATTACK:
    {
      // check if enough time has passed to do something
      if (millis() - C[chan].timeBase >= C[chan].timeStep)
      {
        // if the current level was the end of the interval
        if ((C[chan].adsr->invert && C[chan].volume == 0) ||
            (!C[chan].adsr->invert && C[chan].volume == C[chan].adsr->Vmax))
        {
          // set timing parameters for DECAY phase
          C[chan].timeBase = millis();
          C[chan].timeStep = C[chan].adsr->Td / (C[chan].adsr->Vmax - C[chan].adsr->Vs);

          // reverse volume step direction from current one
          C[chan].volumeStep *= -1;

          DEBUGS("\n->ATTACK to DECAY");
          C[chan].state = DECAY;
        }
        else
        {
          DEBUG("\n--ATTACK Volume delta ", C[chan].volumeStep);
          // still in ATTACK, just set the volume to the new level
          setVolume(chan, C[chan].volume + C[chan].volumeStep);
        }
      }
    }
    break;

    case DECAY:
    {
      // check if enough time has passed to do something
      if (millis() - C[chan].timeBase >= C[chan].timeStep)
      {
        // if the current level was the end of the interval
        if (C[chan].volume == C[chan].adsr->Vs)
        {
          C[chan].state = SUSTAIN;
          DEBUGS("\n->DECAY to SUSTAIN");
        }
        else
        {
          DEBUG("\n--DECAY Volume delta ", C[chan].volumeStep);
          // still in DECAY, just set the volume to the new level
          setVolume(chan, C[chan].volume + C[chan].volumeStep);
        }
      }
    }
    break;

    case SUSTAIN:
      // do noting but keep playing the same note at current volume
    break;

    case NOTE_OFF:
    {
      DEBUGS("\n->NOTE_OFF");
      // set timing parameters for RELEASE phase
      C[chan].timeBase = millis();
      C[chan].timeStep = C[chan].adsr->Tr / C[chan].adsr->Vs;

      // volume step direction remains the same as for previous DECAY
      // but we set this explicitely as NOTE_OFF can happen anytime,
      // before we finish ATTACK.
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
        if ((!C[chan].adsr->invert && C[chan].volume == 0) ||
            (C[chan].adsr->invert && C[chan].volume == C[chan].adsr->Vmax))
        {
          DEBUGS("\n->RELEASE to IDLE");
          setVolume(chan, 0);
          C[chan].state = IDLE;
        }
        else
        {
          DEBUG("\n--RELEASE Volume delta ", C[chan].volumeStep);
          // still in RELEASE, just set the volume to the new level
          setVolume(chan, C[chan].volume + C[chan].volumeStep);
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


void MD_SN74689::send(uint8_t data)
// Send data D0-D7. D0 is the MSB
{
  // DEBUGX("\nsend 0x", data);

  digitalWrite(_WE, HIGH);

  // Set the data pins to current value
  for (int8_t i = DATA_BITS - 1; i >= 0; i--)
  {
    uint8_t v = (data & bit(i)) ? HIGH : LOW;
    uint8_t p = DATA_BITS - i - 1;
    // DEBUG("[", p);  DEBUG(":", v);  DEBUGS("]");
    digitalWrite(_D[p], v);
  }
  
  // Toggle !WE LOW then HIGH to latch it in the IC
  digitalWrite(_WE, LOW);
  delayMicroseconds(10);    // 4Mhz clock means 32 cycles for load are about 8us
  digitalWrite(_WE, HIGH);
}

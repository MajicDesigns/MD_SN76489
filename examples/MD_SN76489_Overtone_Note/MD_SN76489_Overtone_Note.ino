// MD_SN74689 Library example program.
//
// Tests effect of overtones playing on different channels.
//

#include <MD_SN76489.h>

// Define if we are using a direct or SPI interface to the sound IC
// 1 = use direct, 0 = use SPI
#ifndef USE_DIRECT
#define USE_DIRECT 1
#endif

// Hardware Definitions ---------------
#if USE_DIRECT
// All the pins directly connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
#else
// Define the SPI related pins
const uint8_t LD_PIN = 10;
const uint8_t DAT_PIN = 11;
const uint8_t CLK_PIN = 13;
#endif
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin

// Global Data ------------------------
#if USE_DIRECT
MD_SN76489_Direct S(D_PIN, WE_PIN, true);
#else
MD_SN76489_SPI S(LD_PIN, DAT_PIN, CLK_PIN, WE_PIN, true);
#endif

// Code -------------------------------
void setup(void)
{
  Serial.begin(57600);
  S.begin();
}

void loop(void)
{
  const uint16_t NOTE_TIME = 500;
  const uint16_t WAIT_TIME = 1000;
  const uint8_t VOL_DELTA = 2;
  const uint16_t NOTE_FREQ = 440;

  enum state_t { PAUSE, PLAY1, PLAY2, PLAY3, WAIT_END };
  static state_t state = PAUSE;
  static state_t nextState = PLAY1;
  static uint32_t timeStart = 0;

  S.play();

  switch (state)
  {
  case PAUSE:   // Wait for WAIT_TIME
    if (millis() - timeStart >= WAIT_TIME)
      state = nextState;
    break;

  case WAIT_END:
    if (S.isIdle(0))
    {
      state = PAUSE;
      timeStart = millis();
    }
    break;

  case PLAY1:
    S.note(0, NOTE_FREQ, MD_SN76489::VOL_MAX, NOTE_TIME);
    state = WAIT_END;
    nextState = PLAY2;
    break;

  case PLAY2:
    S.note(0, NOTE_FREQ, MD_SN76489::VOL_MAX, NOTE_TIME);
    S.note(1, NOTE_FREQ*2, MD_SN76489::VOL_MAX-VOL_DELTA, NOTE_TIME);
    state = WAIT_END;
    nextState = PLAY3;
    break;

  case PLAY3:
    S.note(0, NOTE_FREQ, MD_SN76489::VOL_MAX, NOTE_TIME);
    S.note(1, NOTE_FREQ * 2, MD_SN76489::VOL_MAX - VOL_DELTA, NOTE_TIME);
    S.note(2, NOTE_FREQ * 3, MD_SN76489::VOL_MAX - (2*VOL_DELTA), NOTE_TIME);
    state = WAIT_END;
    nextState = PLAY1;
    break;
  }
}
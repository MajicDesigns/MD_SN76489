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
  const uint16_t WAIT_DELAY = 1500;
  const uint8_t VOL_DELTA = 2;
  const uint16_t BASE_FREQ = 440;

  S.setVolume(0);
  delay(2*WAIT_DELAY);

  S.setFrequency(0, BASE_FREQ);
  S.setVolume(0, MD_SN76489::VOL_MAX);
  delay(WAIT_DELAY);

  S.setFrequency(1, BASE_FREQ * 2);
  S.setVolume(MD_SN76489::VOL_MAX - VOL_DELTA);
  delay(WAIT_DELAY);

  S.setFrequency(2, BASE_FREQ * 3);
  S.setVolume(MD_SN76489::VOL_MAX - (2 * VOL_DELTA));
  delay(WAIT_DELAY);
}
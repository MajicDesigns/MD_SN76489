// MD_SN74689 Library example program.
//
// Tests effect of overtones playing on different channels.
//

#include <MD_SN76489.h>

// Hardware Definitions ---------------
// All the pins connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin

// Global Data ------------------------
MD_SN76489 S(D_PIN, WE_PIN, true);

// Code -------------------------------
void setup(void)
{
  S.begin();
}

void loop(void)
{
  const uint16_t WAIT_DELAY = 2500;
  const uint8_t VOL_DELTA = 3;
  const uint16_t BASE_FREQ = 440;

  S.setVolume(0);
  delay(WAIT_DELAY);

  S.setFrequency(0, BASE_FREQ);
  S.setVolume(0, MD_SN76489::VOL_MAX);
  delay(WAIT_DELAY);

  S.setFrequency(1, BASE_FREQ*2);
  S.setVolume(1, MD_SN76489::VOL_MAX - VOL_DELTA);
  delay(WAIT_DELAY);

  S.setFrequency(2, BASE_FREQ*3);
  S.setVolume(2, MD_SN76489::VOL_MAX - (2 * VOL_DELTA));
  delay(WAIT_DELAY);
}
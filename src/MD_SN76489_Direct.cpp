/*
MD_SN76489 - Library for using a SN74689 sound generator

See header file for copyright and licensing comments.
*/
#include <MD_SN76489.h>

/**
 * \file
 * \brief Derived class MD_SN76489_Direct functions
 */
void MD_SN76489_Direct::begin(void)
{
  // Set all pins to outputs
  for (int8_t i = 0; i < DATA_BITS; i++)
    pinMode(_D[i], OUTPUT);
  pinMode(_we, OUTPUT);

  // Call the base class
  MD_SN76489::begin();
}

void MD_SN76489_Direct::send(uint8_t data)
{
  // DEBUGX("\nsend 0x", data);

  digitalWrite(_we, HIGH);

  // Set the data pins to current value
  for (int8_t i = DATA_BITS - 1; i >= 0; i--)
  {
    uint8_t v = (data & bit(i)) ? HIGH : LOW;
    uint8_t p = DATA_BITS - i - 1;
    // DEBUG("[", p);  DEBUG(":", v);  DEBUGS("]");
    digitalWrite(_D[p], v);
  }

  // Toggle !WE LOW then HIGH to latch it in the IC
  digitalWrite(_we, LOW);
  delayMicroseconds(10);    // 4Mhz clock means 32 cycles for load are about 8us
  digitalWrite(_we, HIGH);
}

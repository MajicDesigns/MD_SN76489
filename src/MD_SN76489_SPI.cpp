/*
MD_SN76489 - Library for using a SN74689 sound generator

See header file for copyright and licensing comments.
*/
#include <MD_SN76489.h>

/**
 * \file
 * \brief Derived class MD_SN76489_SPI functions
 */
void MD_SN76489_SPI::begin(void)
{
  pinMode(_dat, OUTPUT);
  pinMode(_clk, OUTPUT);
  pinMode(_ld, OUTPUT);
  pinMode(_we, OUTPUT);

  // Call the base class
  MD_SN76489::begin();
}

void MD_SN76489_SPI::send(uint8_t data)
{
  // DEBUGX("\nsend 0x", data);

  digitalWrite(_we, HIGH);

  // Send out the data
  digitalWrite(_ld, LOW); // load data
  shiftOut(_dat, _clk, MSBFIRST, data); // send register
  digitalWrite(_ld, HIGH);
  
  // Toggle !WE LOW then HIGH to latch it in the SN76489 IC
  digitalWrite(_we, LOW);
  delayMicroseconds(10);    // 4Mhz clock means 32 cycles for load are about 8us
  digitalWrite(_we, HIGH);
}


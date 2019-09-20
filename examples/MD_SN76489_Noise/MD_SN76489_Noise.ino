// MD_SN74689 Library example program.
//
// Tests noise playing functions of the library.
// Adds in frequency to channel 2 for *_3 noise option.
//

#include <MD_SN76489.h>

// Testing options
#define DURATION_IN_CALL 0 // 1 = duration in function call, 0 = duration managed by application

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

  Serial.println(F("[MD_SN76489 Noise Tester]"));

  S.begin();
  S.setFrequency(2, 3000);
}

void loop(void)
{
  // Timing constants
  const uint16_t PAUSE_TIME = 300;  // pause between note in ms
#if DURATION_IN_CALL
  const uint16_t PLAY_TIME  = 750;  // noise playing time in ms
#else
  const uint16_t PLAY_TIME = 0;     // playing time managed by application
  const uint16_t WAIT_TIME = 750;
#endif

  // Noise cycle
  MD_SN76489::noiseType_t N[] =
  {
    MD_SN76489::PERIODIC_0, MD_SN76489::PERIODIC_1,
    MD_SN76489::PERIODIC_2, MD_SN76489::PERIODIC_3,
    MD_SN76489::WHITE_0, MD_SN76489::WHITE_1,
    MD_SN76489::WHITE_2, MD_SN76489::WHITE_3
  };

  // Note on/off FSM variables
  static enum { PAUSE, NOTE_ON, WAIT_FOR_TIME, NOTE_OFF } state = PAUSE; // current state
  static uint32_t timeStart = 0;  // millis() timing marker

  static uint8_t idxNoise = 0;
  
  S.play(); // run the sound machine every time through loop()

  // Manage the timing of notes on and off depending on 
  // where we are in the rotation/playing cycle
  switch (state)
  {
    case PAUSE: // pause between notes
    {
      if (millis() - timeStart >= PAUSE_TIME)
        state = NOTE_ON;
    }
    break;

    case NOTE_ON:  // play the next noise setting
    {
      S.noise(N[idxNoise], MD_SN76489::VOL_MAX, PLAY_TIME);
      Serial.print(idxNoise);

      // move to next noise cyle value
      idxNoise = (idxNoise + 1) % ARRAY_SIZE(N);
      if (idxNoise == 0) Serial.print("\n");

      // set up the timer for next state
      timeStart = millis();
#if DURATION_IN_CALL
      state = NOTE_OFF;
#else
      state = WAIT_FOR_TIME;
#endif
    }
    break;

    case WAIT_FOR_TIME:
#if !DURATION_IN_CALL
    {
      if (millis() - timeStart >= WAIT_TIME)
      {
        S.noise(MD_SN76489::NOISE_OFF, MD_SN76489::VOL_OFF);
        timeStart = millis();
        state = NOTE_OFF;
      }
    }
#endif
    break;

    case NOTE_OFF:  // wait for time to turn the note off
    {
      if (S.isIdle(MD_SN76489::NOISE_CHANNEL))
      {
        timeStart = millis();
        state = PAUSE;
      }
    }
    break;
  }
}
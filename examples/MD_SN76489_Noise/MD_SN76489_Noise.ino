// MD_SN74689 Library example program.
//
// Tests noise playing functions of the library.
// Adds in frequency to channel 2 for *_3 noise option.
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
  Serial.begin(57600);

  Serial.println(F("[MD_SN76489 Noise Tester]"));

  S.begin();
  S.setFrequency(2, 3000);
}

void loop(void)
{
  // Timing constants
  const uint16_t PAUSE_TIME = 300;  // pause between note in ms
  const uint16_t PLAY_TIME  = 750;  // nose playing time in ms

  // Noise cycle
  MD_SN76489::noiseType_t N[] =
  {
    MD_SN76489::PERIODIC_0, MD_SN76489::PERIODIC_1,
    MD_SN76489::PERIODIC_2, MD_SN76489::PERIODIC_3,
    MD_SN76489::WHITE_0, MD_SN76489::WHITE_1,
    MD_SN76489::WHITE_2, MD_SN76489::WHITE_3
  };

  // Note on/off FSM variables
  static enum { PAUSE, NOTE_ON, NOTE_OFF } state = PAUSE; // current state
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
      S.noise(N[idxNoise]);
      Serial.print(idxNoise);

      // move to next noise cyle value
      idxNoise = (idxNoise + 1) % ARRAY_SIZE(N);
      if (idxNoise == 0) Serial.print("\n");

      // set up the timer for next state
      timeStart = millis();
      state = NOTE_OFF;
    }
    break;

    case NOTE_OFF:  // wait for time to turn the note off
    {
      if (millis() - timeStart >= PLAY_TIME)
      {
        S.noise(MD_SN76489::NOISE_OFF);
        timeStart = millis();
        state = PAUSE;
      }    
    }
    break;
  }
}
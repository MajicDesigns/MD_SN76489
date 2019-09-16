// MD_SN74689 Library example program.
//
// Tests tone/note playing functions of the library.
// Plays MIDI notes 0-127 in sequence over and over again.
//
// Library Dependencies
// MusicTable library located at https://github.com/MajicDesigns/MD_MusicTable
//

#include <MD_SN76489.h>
#include <MD_MusicTable.h>

// Hardware Definitions ---------------
// All the pins connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin
const uint8_t TEST_CHAN = 1;  // Channel being exercised in

// Global Data ------------------------
MD_SN76489 S(D_PIN, WE_PIN, true);
MD_MusicTable T;

// Code -------------------------------
void setup(void)
{
  Serial.begin(57600);
  Serial.println(F("[MD_SN76489 Tester]"));

  S.begin();
}

void loop(void)
{
  // Timing constants
  const uint8_t START_NOTE = 48;    // first sensible frequency
  const uint16_t PAUSE_TIME = 300;  // pause between note in ms
  const uint16_t PLAY_TIME = 500;   // note playing time in ms

  // Note on/off FSM variables
  static enum { PAUSE, NOTE_ON, NOTE_OFF } state = PAUSE; // current state
  static uint32_t timeStart = 0;  // millis() timing marker
  static uint8_t noteId = START_NOTE;  // the next note to play
  
  S.play(); // run the sound machine every time through loop()

  // Manage the timing of notes on and off depending on 
  // where we are in the rotation/playing cycle
  switch (state)
  {
    case PAUSE: // pause between notes
      if (millis() - timeStart >= PAUSE_TIME)
        state = NOTE_ON;
      break;

    case NOTE_ON:  // play the next MIDI note
      if (T.findId(noteId))
      {
        uint16_t f = (uint16_t)(T.getFrequency() + 0.5);  // round it up
        char buf[10];

        Serial.print(F("["));
        Serial.print(noteId);
        Serial.print(F("] "));
        Serial.print(T.getName(buf, sizeof(buf)));
        Serial.print(F(" @ "));
        Serial.print(f);
        Serial.println(F("Hz"));
        S.note(TEST_CHAN, f, PLAY_TIME);
      }

      // wraparound the note number if reached end midi notes
      noteId++;
      if (noteId >= MD_MusicTable::NOTES_COUNT)
        noteId = START_NOTE;

      // next state
      state = NOTE_OFF;
      break;

    case NOTE_OFF:  // wait for note to turn off automatically
      if (S.isIdle(TEST_CHAN))
        state = PAUSE;
      break;
  }
}
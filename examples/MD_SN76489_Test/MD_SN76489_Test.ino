#include <MD_SN76489.h>
#include <MD_MusicTable.h>

#define DEBUG 1

#if DEBUG
#define PRINTS(s) { Serial.print(F(s)); }
#define PRINTX(s, v) { Serial.print(F(s)); Serial.print(v, HEX); }
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }
#else
#define PRINTS(s)
#define PRINTX(s, v)
#define PRINT(s, v)
#endif

const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;

MD_SN74689 S(D_PIN, WE_PIN, true);
MD_MusicTable T;

void setup(void)
{
#if DEBUG
  Serial.begin(57600);
#endif
  PRINTS("\n[MD_SN76489 Tester]");

  S.begin();
}

void loop(void)
{
  static uint32_t timeStart = 0;
  static bool inNote = false;     // to know if we are playing a note or not
  static uint8_t noteId = 0;

  S.run();    // run the note machine

  if (!inNote)
  {
    if (S.isIdle(0))  // ADSR may still be running
    {
      delay(500);     // pause between notes

      // now play the next MIDI note
      if (T.findId(noteId++));
      {
        uint16_t f = (uint16_t)(T.getFrequency() + 0.5);
        char buf[10];

        PRINT("\n", T.getName(buf, sizeof(buf)));
        PRINT(" @ ", f);
        S.play(0, f);
      }

      // wraparound the note number
      if (noteId == 128) noteId = 0;
      inNote = true;
      timeStart = millis();
    }
  }
  else
  {
    // wait for aone second and then turn the note off
    if (millis() - timeStart >= 1000)
    {
      S.play(0, 0);
      inNote = false;
    }
  }
}
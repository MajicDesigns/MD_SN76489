
// MD_SN74689 Library example program.
//
// Plays plays RTTTL (RingTone Text Transfer Language) songs.
// Cycles through all the includes songs in sequence.
//
// RTTTL format definition https://en.wikipedia.org/wiki/Ring_Tone_Transfer_Language
// Lots of RTTTL files at http://www.picaxe.com/RTTTL-Ringtones-for-Tune-Command/
//
// Library Dependencies
// MusicTable library located at https://github.com/MajicDesigns/MD_MusicTable
// RTTTLParser library located at https://github.com/MajicDesigns/MD_RTTTLParser
//

#include <MD_SN76489.h>
#include <MD_RTTTLParser.h>          // RTTTL Parser  
#include <MD_MusicTable.h>           // MIDI notes information
#include "MD_SN76489_RTTTL_Player.h" // RTTTL song data in a separate file

// Define if we are using a direct or SPI interface to the sound IC
// 1 = use direct, 0 = use SPI
#ifndef USE_DIRECT
#define USE_DIRECT 1
#endif

// Define if we are playing with harmonics just single notes
// Additional channels will play 2x and 3x the note frequency
#ifndef USE_HARMONICS
#define USE_HARMONICS 0
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#define PRINT(s,v)  { Serial.print(F(s)); Serial.print(v); }
#define PRINTX(s,v) { Serial.print(F(s)); Serial.print("0x"); Serial.print(v, HEX); }
#define PRINTS(s)   { Serial.print(F(s)); }
#define PRINTC(c)   { Serial.print(c); }
#else
#define PRINT(s,v)
#define PRINTX(s,v)
#define PRINTS(s)
#define PRINTC(c)
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

const uint8_t PLAY_CHAN = 0;    // Main playing channel
const uint8_t PLAY_VOL = MD_SN76489::VOL_MAX;
#if USE_HARMONICS
const uint8_t PLAY_HARM1 = 1;  // Harmonics channel 1
const uint8_t PLAY_HARM2 = 2;  // Harmonics channel 2
const uint8_t HARM_VOL = PLAY_VOL - 3; // HArmincs volume lower by this much from main volume
#endif

// Global Data ------------------------
#if USE_DIRECT
MD_SN76489_Direct S(D_PIN, WE_PIN, true);
#else
MD_SN76489_SPI S(LD_PIN, DAT_PIN, CLK_PIN, WE_PIN, true);
#endif
MD_MusicTable T;
MD_RTTTLParser P;

void RTTTLhandler(uint8_t octave, uint8_t noteId, uint32_t duration, bool activate)
// If activate is true, play a note (octave, noteId) for the duration (ms) specified.
// If activate is false, the note should be turned off (other parameters are ignored).
//
// A deactivate call always follow an activate. If the music output can work with duration, 
// the handler needs to ignore the deactivate callback.
{
  if (activate)
  {
    if (T.findNoteOctave(noteId, octave))
    {
      uint16_t f = (uint16_t)(T.getFrequency() + 0.5);  // round it up
#if DEBUG
      char buf[10];
#endif

      PRINT("\nNOTE_ON ", T.getName(buf, sizeof(buf)));
      PRINT(" @ ", f);
      PRINT("Hz for ", duration);
      PRINTS("ms");
      S.note(PLAY_CHAN, f, PLAY_VOL, duration);
#if USE_HARMONICS
      S.note(PLAY_HARM1, f * 2, HARM_VOL, duration);
      S.note(PLAY_HARM2, f * 3, HARM_VOL, duration);
#endif
    }
  }
}

void setup(void)
{
  Serial.begin(57600);
  PRINTS("\n[MD_SN76489 RTTL Player]");

  S.begin();
  P.begin();
  P.setCallback(RTTTLhandler);
}

void loop(void)
{
  // Note on/off FSM variables
  const uint16_t PAUSE_TIME = 1500;  // pause time between melodies

  static enum { START, PLAYING, WAIT_BETWEEN } state = START; // current state
  static uint32_t timeStart = 0;    // millis() timing marker
  static uint8_t idxTable = 0;      // index of next song to play

  S.play(); // run the sound machine every time through loop()

  // Manage reading and playing the note
  switch (state)
  {
    case START: // starting a new melody
      P.setTune_P(songTable[idxTable]);
      Serial.print("\n");
      Serial.print(P.getTitle());

      // set up for next song
      idxTable++;
      if (idxTable == ARRAY_SIZE(songTable)) 
        idxTable = 0;

      state = PLAYING;
      break;

    case PLAYING:     // playing a melody - get next note
      if (P.run())
      {
        timeStart = millis();
        state = WAIT_BETWEEN;
      }
      break;

    case WAIT_BETWEEN:  // wait at the end of a melody
      if (millis() - timeStart >= PAUSE_TIME)
        state = START;   // start a new melody
      break;
  }
}
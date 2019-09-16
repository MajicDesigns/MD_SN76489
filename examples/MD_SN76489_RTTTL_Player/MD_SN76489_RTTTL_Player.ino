
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
//

#include <MD_SN76489.h>
#include <MD_MusicTable.h>          // MIDI notes information
#include "MD_SN76489_RTTTL_Player.h" // RTTL song data in a separate file

#define DEBUG 0

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
// All the pins connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin
const uint8_t PLAY_CHAN = 1;  // Channel being exercised

// Global Data ------------------------
MD_SN76489 S(D_PIN, WE_PIN, true);
MD_MusicTable T;

// values for the current RTTL string
uint8_t dDuration = 4;  // duration in ms for one note
uint8_t dOctave = 6;    // default octave scale
uint32_t timeNote;      // time in ms for a whole note

// RTTL Processing --------------------
// Current song string processing functions, operating on global data
const char RTTL_SECTION_DELIMITER = ':';
const char RTTL_FIELD_DELIMITER = ',';
const char RTTL_FIELD_EQUATE = '=';
const char RTTL_NOTE_SHARP = '#';
const char RTTL_NOTE_DOTTED = '.';

const char* pSong;
uint16_t curIdx;
bool eoln;

#define isEoln()      (eoln)
#define setBuffer(p)  { pSong = p; curIdx = 0; eoln = false; }
#define ungetCh(c)    { curIdx--; }
#define isdigit(c)    (c >= '0' && c <= '9')

char getCh(void)
// return the next char, skipping whitespace
{
  char c;

  do 
  {
    c = pgm_read_byte(&pSong[curIdx]);

    eoln = (c == '\0');
    if (!eoln) curIdx++;
  } while (!eoln && isspace(c));

  return(c);
}

uint16_t getNum()
// get the next number
{
  uint16_t num = 0;
  char c = getCh();

  while (isdigit(c))
  {
    num = (num * 10) + (c - '0');
    c = getCh();
  }
  ungetCh(c);

  return(num);
}

void synch(char cSync)
// resynch to past this character in the stream
{
  char c;

  do
  {
    c = getCh();
  } while (c != '\0' && c != cSync);
}

void processRTTLHeader(void)
{
  char c;
  char name[30];
  char* p = name;
  int16_t bpm = 63;

  // skip name
  PRINTS("\n>SKIP NAME ");
  do {
    c = getCh();
    *p++ = c;
  } while (c != RTTL_SECTION_DELIMITER);
  *p = '\0';
  PRINTS("\n");
  Serial.println(name);

  // Now handle defaults section
  // format: d=N,o=N,b=NNN:
  PRINTS("\n>DEFAULTS ");
  do
  {
    c = getCh();
    switch (c)
    {
    case 'd':    // get default duration
      synch(RTTL_FIELD_EQUATE);
      dDuration = getNum();
      synch(RTTL_FIELD_DELIMITER);
      PRINT("\ndDur: ", dDuration);
      break;

    case 'o':   // get default octave
      synch(RTTL_FIELD_EQUATE);
      dOctave = getNum();
      synch(RTTL_FIELD_DELIMITER);
      PRINT("\ndOct: ", dOctave);
      break;

    case 'b':   // get BPM
      synch(RTTL_FIELD_EQUATE);
      bpm = getNum();
      // synch(RTTL_SECTION_DELIMITER);
      PRINT("\nBPM: ", bpm);
      // BPM usually expresses the number of quarter notes per minute
      timeNote = (60 * 1000L / bpm) * 4;  // this is the time for whole note (in milliseconds)
      PRINT(" -> note time (ms): ", timeNote);
      break;
    }
  } while (c != RTTL_SECTION_DELIMITER);
}

uint16_t nextRTTLNote(uint8_t &midiNote, uint16_t &duration)
{
  char c;
  char n[3];
  uint16_t num;
  uint16_t note;
  uint8_t octave;

 // first, get note playDuration, if available
 // we will need to check if we are a dotted note after  
  num = getNum();
  duration = (num != 0) ? (timeNote / num) : (timeNote / dDuration); 

  // now get the note into a string (with sharp if present)
  note = 0;
  n[0] = toupper(getCh());
  n[1] = getCh();    // check for optional sharp
  n[2] = '\0';
  if (n[1] != RTTL_NOTE_SHARP)
  {
    ungetCh();
    n[1] = '\0';
  }
  if (n[0] != 'P')
    note = T.lookupNoteId(n);

  // check for optional dotted
  c = getCh();
  if (c == RTTL_NOTE_DOTTED)
  {
    duration += duration / 2;
    c = getCh();
  }

  // get the octave
  ungetCh();
  octave = (isdigit(c)) ? getNum() : dOctave;
  synch(RTTL_FIELD_DELIMITER);

  // now calculate the midi note
  PRINT("\n>NEXT NOTE: N=", note);
  PRINT(" (", n);
  PRINT(") O=", octave);
  PRINT(" D=", duration);
  if (n[0] != 'P')
    midiNote = (octave * MD_MusicTable::NOTES_IN_OCTAVE) + note;
  else
    midiNote = 0;
  PRINT("ms --> MIDI ", midiNote);

  return(note);
}

void setup(void)
{
  Serial.begin(57600);
  PRINTS("\n[MD_SN76489 RTTL Player]");

  S.begin();
}

void loop(void)
{
  // Note on/off FSM variables
  const uint16_t PAUSE_TIME = 1500;  // pause time between melodies

  static enum { IDLE, PLAYING, NOTE_ON, PAUSE, WAIT } state = IDLE; // current state
  static uint32_t timeStart = 0;    // millis() timing marker
  static uint8_t midiNote;          // next MIDI note to play
  static uint16_t playDuration;     // note duration
  static uint8_t idxTable = 0;      // index of next song to play

  S.play(); // run the sound machine every time through loop()

  // Manage reading and playing the note
  switch (state)
  {
    case IDLE: // starting a new melody
      setBuffer(songTable[idxTable]);
      // set up for next song
      idxTable++;
      if (idxTable == ARRAY_SIZE(songTable)) 
        idxTable = 0;

      processRTTLHeader();
      state = PLAYING;
      break;

    case PLAYING:     // playing a melody - get a note
      if (S.isIdle(PLAY_CHAN))
      {
        nextRTTLNote(midiNote, playDuration);
        state = NOTE_ON;
      }
      break;

    case NOTE_ON:  // play the next MIDI note
      if (midiNote == 0)     // this is just a pause
      {
        timeStart = millis();
        state = PAUSE;
      }
      else
      {
        if (T.findId(midiNote));
        {
          uint16_t f = (uint16_t)(T.getFrequency() + 0.5);  // round it up
#if DEBUG
          char buf[10];
#endif

          PRINT("\nNOTE_ON ", T.getName(buf, sizeof(buf)));
          PRINT(" @ ", f);
          PRINTS("Hz");
          S.note(PLAY_CHAN, f, playDuration);
        }
      }

      // set up the timer for next state
      timeStart = millis();
      state = isEoln() ? WAIT : PLAYING;
      break;

    case PAUSE:  // pause note during melody
      if (millis() - timeStart >= playDuration)
        state = PLAYING;   // start a new melody
      break;

    case WAIT:  // wait at the end of a melody
      if (millis() - timeStart >= PAUSE_TIME)
        state = IDLE;   // start a new melody
      break;
  }
}
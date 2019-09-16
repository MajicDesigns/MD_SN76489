// Command Line Interface (CLI or Serial) to play MIDI files from the SD card.
// 
// Enter commands on the serial monitor to control the application
//
// Dependencies
// SDFat at https://github.com/greiman?tab=repositories
// MD_MIDIFile at https://github.com/MajicDesigns/MD_MIDIFile
// MD_MusicTable at https://github.com/MajicDesigns/MD_MusicTable
//

#include <SdFat.h>
#include <MD_MIDIFile.h>
#include <MD_SN76489.h>
#include <MD_MusicTable.h>

// Hardware Definitions ---------------
// All the pins connected to D0-D7 on the IC, in sequential order 
// so that pin D_PIN[0] is connected to D0, D_PIN[1] to D1, etc.
const uint8_t D_PIN[] = { A0, A1, A2, A3, 4, 5, 6, 7 };
const uint8_t WE_PIN = 8;     // Arduino pin connected to the IC WE pin
const uint8_t MAX_SND_CHAN = MD_SN76489::MAX_CHANNELS - 1;

// SD chip select pin for SPI comms.
// Default SD chip select is the SPI SS pin (10 on Uno, 53 on Mega).
const uint8_t SD_SELECT = SS;

// Miscellaneous
const uint8_t RCV_BUF_SIZE = 20;      // UI character buffer size
void(*hwReset) (void) = 0;            // declare reset function @ address 0

// Global Data ------------------------
SdFat SD;
MD_MIDIFile SMF;
MD_SN76489 S(D_PIN, WE_PIN, true);
MD_MusicTable T;

bool printMidiStream = true;   // flag to print the real time midi stream
char rcvBuf[RCV_BUF_SIZE];  // buffer for characters received from the console

struct channelData
{
  bool idle;
  uint8_t track;
  uint8_t chan;
  uint8_t note;
} chanData[MAX_SND_CHAN];

int8_t findChan(uint8_t track, uint8_t chan, uint8_t note)
{
  uint8_t c = -1;

  for (uint8_t i = 0; i < MAX_SND_CHAN; i++)
    if (chanData[i].track == track && 
        chanData[i].chan == chan && 
        chanData[i].note == note)
    {
      c = i;
      break;
    }

  return(c);
}

int8_t findFreeChan(void)
{
  int8_t c = -1;

  for (uint8_t i = 0; i < MAX_SND_CHAN; i++)
    if (chanData[i].idle)
    {
      c = i;
      break;
    }

  return(c);
}

void midiCallback(midi_event* pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
// Data is passed using a pointer to this structure:
// typedef struct
//{
//  uint8_t track;    ///< the track this was on
//  uint8_t channel;  ///< the midi channel
//  uint8_t size;     ///< the number of data bytes
//  uint8_t data[4];  ///< the data. Only 'size' bytes are valid
//} midi_event;
{
  int8_t c;

  // Print the data if enabled
  if (printMidiStream)
  {
    Serial.print(F("\nT"));
    Serial.print(pev->track);
    Serial.print(F(": Ch "));
    Serial.print(pev->channel + 1);
    Serial.print(F(" Data"));
    for (uint8_t i = 0; i < pev->size; i++)
    {
      Serial.print(F(" "));
      if (pev->data[i] <= 0x0f)
        Serial.print(F("0"));
      Serial.print(pev->data[i], HEX);
    }
  }

  // Set the MIDI data to the hardware
  switch (pev->data[0])
  {
  case 0x80:  // Note off
    c = findChan(pev->track, pev->channel, pev->data[1]);
    if (c == -1) break;

    S.note(c, 0);
    if (printMidiStream) 
    {
      Serial.print(F(" -> NOTE OFF C")); 
      Serial.print(c);
    }
    break;

  case 0x90:  // Note on
    if (pev->data[2] == 0)    // velocity == 0 -> note off
    {
      c = findChan(pev->track, pev->channel, pev->data[1]);
      if (c == -1) break;

      S.note(c, 0);
      if (printMidiStream)
      {
        Serial.print(F(" -> NOTE OFF C"));
        Serial.print(c);
      }
    }
    else
    {
      c = findFreeChan();
      if (c == -1) break;

      if (T.findId(pev->data[1]))
      {
        uint8_t v = map(pev->data[2], 0, 0x7f, 0, 0xf);
        uint16_t f = (uint16_t)(T.getFrequency() + 0.5);  // round it up
        char buf[10];
        
        S.setVolume(c, v);
        S.note(c, f);
        chanData[c].idle = false;
        chanData[c].chan = pev->channel;
        chanData[c].track = pev->track;
        chanData[c].note = pev->data[1];
        if (printMidiStream)
        {
          Serial.print(F(" -> NOTE ON C"));
          Serial.print(c);
          Serial.print(F(" ["));
          Serial.print(T.getId());
          Serial.print(F("] "));
          Serial.print(T.getName(buf, sizeof(buf)));
          Serial.print(F(" @ "));
          Serial.print(f);
          Serial.print(F("Hz V"));
          Serial.print(v);
        }
      }
    }
    break;
  }
}

void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  S.setVolume(MD_SN76489::VOL_OFF);
}

void help(void)
{
  Serial.print(F("\nh,?\thelp"));
  Serial.print(F("\nf fldr\tset current folder to fldr"));
  Serial.print(F("\nl\tlist out files in current folder"));
  Serial.print(F("\np file\tplay the named file"));

  Serial.print(F("\n\n* Sketch Control"));
  Serial.print(F("\nz s\tsoftware reset"));
  Serial.print(F("\nz m\tmidi silence"));
  Serial.print(F("\nz d\tdump the real time midi stream (toggle on/off)"));

  Serial.print(F("\n"));
}

const char *SMFErr(int err)
{
  const char *DELIM_OPEN = "[";
  const char *DELIM_CLOSE = "]";

  static char szErr[30];
  char szFmt[10];

  static const char PROGMEM E_OK[] = "\nOK";  // -1
  static const char PROGMEM E_FILE_NUL[] = "Empty filename"; // 0 
  static const char PROGMEM E_OPEN[] = "Cannot open"; // 1
  static const char PROGMEM E_FORMAT[] = "Not MIDI"; // 2
  static const char PROGMEM E_HEADER[] = "Header size"; // 3
  static const char PROGMEM E_FILE_FMT[] = "File unsupproted"; // 5
  static const char PROGMEM E_FILE_TRK0[] = "File fmt 0; trk > 1"; // 6
  static const char PROGMEM E_MAX_TRACK[] = "Too many tracks"; // 7
  static const char PROGMEM E_NO_CHUNK[] = "no chunk"; // n0
  static const char PROGMEM E_PAST_EOF[] = "past eof"; // n1
  static const char PROGMEM E_UNKNOWN[] = "Unknown";

  static const char PROGMEM EF_ERR[] = "\n%d %s";    // error number
  static const char PROGMEM EF_TRK[] = "Trk %d,";  // for errors >= 10

  if (err == -1)    // this is a simple message
    strcpy_P(szErr, E_OK);
  else              // this is a complicated message
  {
    strcpy_P(szFmt, EF_ERR);
    sprintf(szErr, szFmt, err, DELIM_OPEN);
    if (err < 10)
    {
      switch (err)
      {
      case 0: strcat_P(szErr, E_FILE_NUL); break;
      case 1: strcat_P(szErr, E_OPEN); break;
      case 2: strcat_P(szErr, E_FORMAT); break;
      case 3: strcat_P(szErr, E_HEADER); break;
      case 4: strcat_P(szErr, E_FILE_FMT); break;
      case 5: strcat_P(szErr, E_FILE_TRK0); break;
      case 6: strcat_P(szErr, E_MAX_TRACK); break;
      case 7: strcat_P(szErr, E_NO_CHUNK); break;
      default: strcat_P(szErr, E_UNKNOWN); break;
      }
    }
    else      // error encoded with a track number
    {
      char szTemp[10];

      // fill in the track number
      strcpy_P(szFmt, EF_TRK);
      sprintf(szTemp, szFmt, err / 10);
      strcat(szErr, szTemp);

      // now do the message
      switch (err % 10)
      {
      case 0: strcat_P(szErr, E_NO_CHUNK); break;
      case 1: strcat_P(szErr, E_PAST_EOF); break;
      default: strcat_P(szTemp, E_UNKNOWN); break;
      }
    }
    strcat(szErr, DELIM_CLOSE);
  }

  return(szErr);
}

bool recvLine(void) 
{
  const char endMarker = '\n'; // end of the Serial input line
  static byte ndx = 0;
  char c;
  bool b = false;        // true when we have a complete line

  while (Serial.available() && !b) // process all available characters before eoln
  {
    c = Serial.read();
    if (c != endMarker) // is the character not the end of the string termintator?
    {
      if (!isspace(c))  // filter out all the whitespace
      {
        rcvBuf[ndx] = toupper(c); // save the character
        ndx++;
        if (ndx >= RCV_BUF_SIZE) // handle potential buffer overflow
          ndx--;
        rcvBuf[ndx] = '\0';       // alwaqys maintain a valid string
      }
    }
    else
    {
      ndx = 0;          // reset buffer to receive the next line
      b = true;         // return this flag
    }
  }
  return(b);
}

void processUI(void)
{
  if (!recvLine())
    return;

  // we have a line to process
  switch (rcvBuf[0])
  {
  case 'H':
  case '?':
    help();
    break;

  case 'Z':   // resets
    switch (rcvBuf[1])
    {
    case 'S': hwReset(); break;
    case 'M': midiSilence(); Serial.print(SMFErr(-1)); break;
    case 'D': printMidiStream = !printMidiStream; Serial.print(SMFErr(-1)); break;
    }
    break;

  case 'P': // play
    {
      int err;

      // clean up current environment
      SMF.close(); // close old MIDI file
      midiSilence(); // silence hanging notes

      Serial.print(F("\nRead File: "));
      Serial.print(&rcvBuf[1]);
      SMF.setFilename(&rcvBuf[1]); // set filename
      Serial.print(F("\nSet file : "));
      Serial.print(SMF.getFilename());
      delay(100);
      err = SMF.load(); // load the new file
      Serial.print(SMFErr(err));
    }
    break;

  case 'F': // set the current folder for MIDI files
    {
      Serial.print(F("\nFolder: "));
      Serial.print(&rcvBuf[1]);
      SMF.setFileFolder(&rcvBuf[1]); // set folder
    }
    break;

  case 'L': // list the files in the current folder
    {
      SdFile file;    // iterated file

      SD.vwd()->rewind();
      while (file.openNext(SD.vwd(), O_READ))
      {
        if (file.isFile())
        {
          char buf[20];

          file.getName(buf, ARRAY_SIZE(buf));
          Serial.print(F("\n"));
          Serial.print(buf);
        }
        file.close();
      }
      Serial.print(F("\n"));
    }
    break;
  }
}

void setup(void) // This is run once at power on
{
  Serial.begin(57600); // For Console I/O
  Serial.print(F("\n[MD_SN74689 Midi Player CLI]"));
  Serial.print(F("\nEnsure serial monitor line ending is set to newline."));

  // Initialise SN74689
    S.begin();

  // Initialize SD
  if (!SD.begin(SD_SELECT, SPI_FULL_SPEED))
  {
    Serial.print(F("\nSD init fail!"));
    while (true);
  }

  // Initialize MIDIFile
  SMF.begin(&SD);
  SMF.setMidiHandler(midiCallback);
  midiSilence(); // Silence any hanging notes

  // show the available commands
  help();
}

void loop(void)
{
  S.play();
  for (uint8_t i = 0; i < MAX_SND_CHAN; i++)
    chanData[i].idle = S.isIdle(i);

  if (!SMF.isEOF()) 
    SMF.getNextEvent(); // Play MIDI data

  processUI();  // process the User Interface
}